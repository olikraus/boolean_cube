/*

  fcgi_bcc.c
  
  Boolean Cube Calculator
  (c) 2026 Oliver Kraus         
  https://github.com/olikraus/boolean_cube

  License: CC BY-SA Attribution-ShareAlike 4.0 International
  https://creativecommons.org/licenses/by-sa/4.0/
  
  indent -bl -bli0 -i2 -nut -npsl -npcs -l140 fcgi_bcc.c
  
*/
#include <fcgi_stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include "co.h"
#include "bc.h"


/* --- Configuration Macros --- */
#define LAYOUT_VERSION 20260403 /* Increment this to force a global reset */
#define SHM_NAME "/fastcgi_config_ram"
#define CONFIG_PATH "/tmp/config.json"


#define MAX_CONFIG_SIZE (1024*128)


/* --- Shared Memory Structure --- */
typedef struct
{
  int layout_version;           /* Version tracking for hot-swapping */
  pthread_rwlock_t lock;
  char json_data[MAX_CONFIG_SIZE];
  int config_len;               /* Length of the configuration data */
  int is_initialized;

  // Global Counters (Shared across all worker processes)
  unsigned long update_count;   /* Successful config updates */
  unsigned long update_wait_count;      /* Times an update had to wait for tasks */
  unsigned long task_wait_count;        /* Times a task had to wait for an update */
} shared_config_t;

shared_config_t *config = NULL;
int local_call_count = 0;
const size_t shm_size = sizeof(shared_config_t);

/* --- Helper: Load Disk Data to RAM --- */
void load_from_disk()
{
  FILE *f = fopen(CONFIG_PATH, "r");
  if (f)
  {
    size_t len = fread(config->json_data, 1, MAX_CONFIG_SIZE - 1, f);
    config->json_data[len] = '\0';
    config->config_len = len;
    fclose(f);
    fprintf(stderr, "[fcgi_task] RAM populated from %s\n", CONFIG_PATH);
  }
  else
  {
    strcpy(config->json_data, "{\"status\": \"initialized_fresh\"}");
    config->config_len = strlen(config->json_data);
    fprintf(stderr, "[fcgi_task] No disk file. RAM initialized with default JSON.\n");
  }
}

/* --- Helper: Setup/Repair Shared Memory --- */
void init_shared_memory()
{
  // 1. If already mapped to old version, unmap first
  if (config != NULL)
  {
    munmap(config, shm_size);
    config = NULL;
  }

  // 2. Open/Create the shared memory file
  int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1)
  {
    perror("shm_open");
    exit(1);
  }

  ftruncate(shm_fd, shm_size);
  config = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  close(shm_fd);

  // 3. Logic: Is it brand new or a version mismatch?
  if (config->layout_version == 0)
  {
    // Fresh initialization
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_rwlock_init(&config->lock, &attr);

    pthread_rwlock_wrlock(&config->lock);
    config->layout_version = LAYOUT_VERSION;
    config->update_count = 0;
    config->update_wait_count = 0;
    config->task_wait_count = 0;
    config->config_len = 0;
    load_from_disk();
    config->is_initialized = 1;
    pthread_rwlock_unlock(&config->lock);
    fprintf(stderr, "[fcgi_task] SHM v%d created and initialized.\n", LAYOUT_VERSION);
  }
  else if (config->layout_version != LAYOUT_VERSION)
  {
    // Version mismatch detected
    fprintf(stderr, "[fcgi_task] Version mismatch (RAM:%d vs Bin:%d). Wiping SHM...\n", config->layout_version, LAYOUT_VERSION);
    shm_unlink(SHM_NAME);
    init_shared_memory();       // Recurse to create the fresh segment
  }
}


/* --- Task Execution --- */


int bcc_task(const char *config_json, const char *task_json)
{
  /* Process configuration and task JSON here */
  co config_co;
  co task_co;
  co all_co = coNewVector(CO_NONE);
  co out_co;
  long i, cnt;
  
  config_co = coReadJSONByString(config_json);
  if ( coIsVector(config_co) == 0 ) 
    return coDelete(config_co), 0;
  
  task_co = coReadJSONByString(task_json);
  if ( coIsVector(task_co) == 0 ) 
    return coDelete(config_co), coDelete(task_co), 0;
  
  cnt = coVectorSize(config_co);
  for( i = 0; i < cnt; i++ )
    if ( coVectorAdd(all_co, coVectorGet(config_co, i)) < 0 )
      return coDelete(all_co), coDelete(config_co), coDelete(task_co), 0;

  cnt = coVectorSize(task_co);
  for( i = 0; i < cnt; i++ )
    if ( coVectorAdd(all_co, coVectorGet(task_co, i)) < 0 )
      return coDelete(all_co), coDelete(config_co), coDelete(task_co), 0;

  out_co = bc_ExecuteVector(all_co);
  coWriteJSON(out_co, /* compact */ 1, 1, stdout); // isUTF8 is 0, then output char codes >=128 via \u 

  return coDelete(out_co), coDelete(all_co), coDelete(config_co), coDelete(task_co), 0;
}

int main(void)
{
  init_shared_memory();

  while (FCGI_Accept() >= 0)
  {
    local_call_count++;

    // --- Self-Healing Check ---
    // If another process updated the version, re-attach before processing
    if (config->layout_version != LAYOUT_VERSION)
    {
      fprintf(stderr, "[fcgi_task] PID %d detected updated SHM layout. Re-mapping...\n", getpid());
      init_shared_memory();
    }

    char *query = getenv("QUERY_STRING");
    char *method = getenv("REQUEST_METHOD");

    // --- 1. MODE: CONFIG UPDATE (Writer) ---
    if (query && strcmp(query, "mode=update") == 0 && strcmp(method, "POST") == 0)
    {
      int blocked = 0;
      if (pthread_rwlock_trywrlock(&config->lock) != 0)
      {
        blocked = 1;
        __sync_fetch_and_add(&config->update_wait_count, 1);
        pthread_rwlock_wrlock(&config->lock);
      }

      int len = atoi(getenv("CONTENT_LENGTH") ? getenv("CONTENT_LENGTH") : "0");
      if (len < 0)
        len = 0;
      if (len > MAX_CONFIG_SIZE - 1)
        len = MAX_CONFIG_SIZE - 1;
      size_t read_len = fread(config->json_data, 1, len, stdin);
      config->json_data[read_len] = '\0';
      config->config_len = read_len;

      // Sync to disk
      FILE *f = fopen(CONFIG_PATH, "w");
      if (f)
      {
        fprintf(f, "%s", config->json_data);
        fclose(f);
      }

      __sync_fetch_and_add(&config->update_count, 1);
      fprintf(stderr, "[fcgi_task] Update successful (Contention: %s)\n", blocked ? "Yes" : "No");

      printf("Content-type: text/plain\r\n\r\nUpdate Successful (v%d).\n", LAYOUT_VERSION);
      pthread_rwlock_unlock(&config->lock);
    }

    // --- 2. MODE: TASK EXECUTION (Reader) ---
    else if (query && strcmp(query, "mode=task") == 0  && strcmp(method, "POST") == 0)
    {
      if (pthread_rwlock_tryrdlock(&config->lock) != 0)
      {
        __sync_fetch_and_add(&config->task_wait_count, 1);
        pthread_rwlock_rdlock(&config->lock);
      }

      int len = atoi(getenv("CONTENT_LENGTH") ? getenv("CONTENT_LENGTH") : "0");
      if (len < 0)
        len = 0;
      char *task_data = (char *)malloc(len + 1);
      if (task_data)
      {
        size_t read_len = fread(task_data, 1, len, stdin);
        task_data[read_len] = '\0';

        bcc_task(config->json_data, task_data);

        printf("Content-type: text/plain\r\n\r\nExecuting Task (PID %d)\n", getpid());
        free(task_data);
      }
      else
      {
        printf("Status: 500 Internal Server Error\r\nContent-type: text/plain\r\n\r\nOut of memory\n");
      }

      pthread_rwlock_unlock(&config->lock);
    }

    // --- 3. DASHBOARD (Observer) ---
    else
    {
      pthread_rwlock_rdlock(&config->lock);
      printf("Content-type: text/html\r\n\r\n");
      printf("<html><head><style>"
             "body{font-family:sans-serif; background:#f4f7f6; padding:40px;}"
             ".container{max-width:900px; margin:auto; background:white; padding:30px; border-radius:12px; box-shadow:0 10px 25px rgba(0,0,0,0.05);}"
             "table{width:100%%; border-collapse:collapse; margin:20px 0;}"
             "th, td{text-align:left; padding:12px; border-bottom:1px dotted #ccc;}"
             "th{background:#f8f9fa;}"
             "pre{background:#2d2d2d; color:#61dafb; padding:15px; border-radius:6px; overflow:auto;}"
             ".status-tag{background:#e1f5fe; color:#01579b; padding:4px 8px; border-radius:4px; font-size:0.8em;}"
             "</style></head><body><div class='container'>");

      printf("<h1>FastCGI Node Monitor <span class='status-tag'>Active</span></h1>");

      printf("<table>"
             "<tr><th>Metric Group</th><th>Field</th><th>Value</th></tr>"
             "<tr><td><b>Version Control</b></td><td>Layout Version</td><td><code>%d</code></td></tr>"
             "<tr><td><b>Worker Process</b></td><td>Current PID</td><td>%d</td></tr>"
             "<tr><td></td><td>Calls to this PID</td><td>%d</td></tr>"
             "<tr><td><b>Global Activity</b></td><td>Total Updates</td><td>%lu</td></tr>"
             "<tr><td><b>Configuration</b></td><td>Config Length</td><td>%d bytes</td></tr>"
             "<tr><td><b>Locking Stats</b></td><td>Update Block Events</td><td><b style='color:#d32f2f;'>%lu</b></td></tr>"
             "<tr><td></td><td>Task Block Events</td><td><b style='color:#f57c00;'>%lu</b></td></tr>"
             "</table>",
             config->layout_version, getpid(), local_call_count, config->update_count, config->config_len, config->update_wait_count, config->task_wait_count);

      printf("<h3>Global RAM Configuration:</h3><pre>%s</pre>", config->json_data);
      printf("</div></body></html>");
      pthread_rwlock_unlock(&config->lock);
    }
  } // while FCGI
  return 0;
}
