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
#include <ctype.h>
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
#ifdef __GLIBC__
    #include <malloc.h>
#endif

/*=================================================================*/

/* 
  Returns the approximate total Kilo-bytes  (1024) of peak heap usage.
  Returns 0 if not on a GLIBC system.
*/
unsigned long get_heap_usage() 
{
#ifndef __GLIBC__
    /* Return 0 for non-GLIBC systems  */
    return 0UL;
#else
    /* Check for glibc 2.33+ to use mallinfo2 */
    #if defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33))
        struct mallinfo2 mi = mallinfo2();
        /* mi.uordblks is the total allocated space in bytes */
        return (unsigned long)(mi.uordblks/1024);
    #else
        /* Fallback for older SLES / GLIBC < 2.33 */
        struct mallinfo mi = mallinfo();
        /* Note: in old mallinfo, uordblks is 'int', so we cast to 
           unsigned to handle values up to 2GB properly. */
        return (unsigned long)((unsigned int)mi.uordblks/1024);
    #endif
#endif
}


/*=================================================================*/
/* Query String Parser */

#define URL_QUERY_PAIR_MAX 16

/* one key value pair for the breakdown of the query string: k1=v1&k2=v2... */
typedef struct
{
  char *key;
  char *val;
} query_pair_t;

query_pair_t query_pairs[URL_QUERY_PAIR_MAX];



/* Converts a hex character to its integer value. */
static int hex_to_int(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

/*
 Decodes a URL-encoded string in-place.
 Handles '+' as space and '%XX' hex sequences.
*/
void url_decode(char *s)
{
  char *src = s;
  char *dst = s;
  while (*src)
  {
    if (*src == '+')
    {
      *dst++ = ' ';
      src++;
    }
    else if (*src == '%' && isxdigit(src[1]) && isxdigit(src[2]))
    {
      int high = hex_to_int(src[1]);
      int low = hex_to_int(src[2]);
      *dst++ = (char) ((high << 4) | low);
      src += 3;
    }
    else
    {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

void init_query()
{
  for (int i = 0; i < URL_QUERY_PAIR_MAX; i++)
  {
    query_pairs[i].key = NULL;
    query_pairs[i].val = NULL;
  }
}

void free_query()
{
  for (int i = 0; i < URL_QUERY_PAIR_MAX; i++)
  {
    if ( query_pairs[i].key != NULL )
      free(query_pairs[i].key);
    if ( query_pairs[i].val != NULL )
      free(query_pairs[i].val);
    query_pairs[i].key = NULL;
    query_pairs[i].val = NULL;
  }
}

/*
  int parse_query()

  get the query string from the environment variable and return a key/value pairs in the global array query_pairs.
  returns the number of key/value pairs (filled elements in the query_pairs array)

  requires a prior call to init_query() and a call ti free_query() after the array content has been processed.
*/
int parse_query()
{
  char *query_env = getenv("QUERY_STRING");
  if (!query_env || strlen(query_env) == 0)
  {
    return 0;
  }

  // Work on a copy because strsep modifies the string
  char *query_copy = strdup(query_env);
  if (!query_copy)
    return 0;

  char *token;
  char *rest = query_copy;
  int count = 0;

  // Outer loop: Split by '&'
  while ((token = strsep(&rest, "&")) != NULL && count < URL_QUERY_PAIR_MAX - 1)
  {
    if (*token == '\0')
      continue;                 // Handle empty pairs like &&

    char *key_part = strsep(&token, "=");
    char *val_part = token;     // Remaining part after '='

    if (key_part)
    {
      url_decode(key_part);
      query_pairs[count].key = strdup(key_part);
    }

    // If val_part is NULL, the query had "key" instead of "key=val"
    if (val_part)
    {
      url_decode(val_part);
      query_pairs[count].val = strdup(val_part);
    }
    else
    {
      query_pairs[count].val = strdup("");      // Default to empty string
    }

    count++;
  }

  free(query_copy);
  return count;
}

const char *get_query_val(const char *key)
{
  for (int i = 0; i < URL_QUERY_PAIR_MAX; i++)
  {
    if (query_pairs[i].key && strcmp(query_pairs[i].key, key) == 0)
      return query_pairs[i].val;
  }
  return NULL;
}



/*=================================================================*/
/* Shared Memory Configuration Data */


/* --- Configuration Macros --- */
#define LAYOUT_VERSION 20260406 /* Increment this to force a global reset */
#define SHM_NAME "/fastcgi_config_ram"
#define CONFIG_PATH "/tmp/fcgi_bcc_config.json"
#define MAX_CONFIG_SIZE (1024*128)



/* Shared Memory Structure */
typedef struct
{
  int layout_version;           /* Version tracking for hot-swapping */
  
  volatile int active_buffer;   /* 0 or 1, atomic */
  volatile int read_in_progress; /* 0 or 1, atomic flag */

  char json_data[2][MAX_CONFIG_SIZE];
  int config_len[2];            /* Length of the configuration data for each buffer */
  int is_initialized;

  // Global Counters (Shared across all worker processes)
  unsigned long update_count;   /* Successful config updates */
  unsigned long task_count;     /* Successful task executions */
  unsigned long update_wait_count;      /* Times an update had to wait for lock (deprecated) */
  unsigned long task_wait_count;        /* Times a task had to wait for lock (deprecated) */
  unsigned long disk_read_count;        /* Number of times read_config_data_from_disk was called */
} shared_config_t;

shared_config_t *config = NULL;
int local_call_count = 0;
const size_t shm_size = sizeof(shared_config_t);


/* Helper: Load Disk Data to RAM with Double Buffering and Atomic Swap */
void read_config_data_from_disk()
{
  // 1. If the read_config_data_in_progress flag is set, then wait until it is cleared and exit
  // Use atomic test-and-set to ensure only one process performs the read
  if (__sync_lock_test_and_set(&config->read_in_progress, 1))
  {
    // Someone else is reading. Wait until they finish.
    while (config->read_in_progress)
    {
      usleep(1000); // 1ms wait
    }
    return;
  }

  // 2. set the read_config_data_in_progress flag (Done by test_and_set above)
  __sync_fetch_and_add(&config->disk_read_count, 1);

  // Determine which buffer to use
  int current_active = config->active_buffer;
  int target_buffer;
  
  // 4. if both buffers are empty, place the file in one buffer and enable that buffer
  // (We use -1 as "uninitialized" state for active_buffer)
  if (current_active < 0)
    target_buffer = 0;
  else
    target_buffer = 1 - current_active;

  // 3. read data from the file, default to "[]" if file doesn't exist
  FILE *f = fopen(CONFIG_PATH, "r");
  if (f)
  {
    size_t len = fread(config->json_data[target_buffer], 1, MAX_CONFIG_SIZE - 1, f);
    config->json_data[target_buffer][len] = '\0';
    config->config_len[target_buffer] = len;
    fclose(f);
    fprintf(stderr, "[bcc.fcgi] Buffer %d populated from %s\n", target_buffer, CONFIG_PATH);
  }
  else
  {
    strcpy(config->json_data[target_buffer], "[]");
    config->config_len[target_buffer] = strlen(config->json_data[target_buffer]);
    fprintf(stderr, "[bcc.fcgi] No disk file. Buffer %d initialized with default JSON.\n", target_buffer);
  }

  // 4. enable that buffer (atomic swap)
  config->active_buffer = target_buffer;

  // 5. clear the read_config_data_in_progress
  __sync_lock_release(&config->read_in_progress);
  fflush(stderr);
}

/* Helper: Setup/Repair Shared Memory */
void init_shared_memory()
{
  fprintf(stderr, "[bcc.fcgi] init_shared_memory\n");
  fflush(stderr);
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
    config->layout_version = LAYOUT_VERSION;
    config->active_buffer = -1; // -1 means no buffer is active yet
    config->read_in_progress = 0;
    config->update_count = 0;
    config->task_count = 0;
    config->disk_read_count = 0;
    config->config_len[0] = 0;
    config->config_len[1] = 0;
    
    read_config_data_from_disk();
    
    config->is_initialized = 1;
    fprintf(stderr, "[bcc.fcgi] SHM v%d created and initialized.\n", LAYOUT_VERSION);
  }
  else if (config->layout_version != LAYOUT_VERSION)
  {
    // Version mismatch detected
    fprintf(stderr, "[bcc.fcgi] Version mismatch (RAM:%d vs Bin:%d). Wiping SHM...\n", config->layout_version, LAYOUT_VERSION);
    shm_unlink(SHM_NAME);
    init_shared_memory();       // Recurse to create the fresh segment
  }
  else
  {
    fprintf(stderr, "[bcc.fcgi] SHM available\n");
  }
}


/*=================================================================*/
/* task execution: bcc call  */

int bcc_task(const char *config_json, const char *task_json)
{
  /* Process configuration and task JSON here */
  co config_co;
  co task_co;
  co all_co = coNewVector(CO_NONE);
  co out_co;
  long i, cnt;

  config_co = coReadJSONByString(config_json);
  if ( config_co == NULL )
  {
    fprintf(stderr, "[bcc.fcgi] Failed to read json config data\n");
    return 0;
  }
  if (coIsVector(config_co) == 0)
  {
    fprintf(stderr, "[bcc.fcgi] Failed: Config data is not a json vector\n");
    return coDelete(config_co), 0;
  }

  task_co = coReadJSONByString(task_json);
  if ( task_co == NULL )
  {
    fprintf(stderr, "[bcc.fcgi] Failed to read json task data\n");
    return coDelete(config_co), 0;
  }
  if (coIsVector(task_co) == 0)
  {
    fprintf(stderr, "[bcc.fcgi] Failed: Task data is not a json vector\n");
    return coDelete(config_co), coDelete(task_co), 0;
  }

  cnt = coVectorSize(config_co);
  for (i = 0; i < cnt; i++)
    if (coVectorAdd(all_co, coVectorGet(config_co, i)) < 0)
      return coDelete(all_co), coDelete(config_co), coDelete(task_co), 0;

  cnt = coVectorSize(task_co);
  for (i = 0; i < cnt; i++)
    if (coVectorAdd(all_co, coVectorGet(task_co, i)) < 0)
      return coDelete(all_co), coDelete(config_co), coDelete(task_co), 0;

  out_co = bc_ExecuteVector(all_co);
  coWriteJSON(out_co, /* compact */ 1, 1, stdout);      // isUTF8 is 0, then output char codes >=128 via \u 

  return coDelete(out_co), coDelete(all_co), coDelete(config_co), coDelete(task_co), 1;
}

/*=================================================================*/
/* fast CGI main loop  */


int main(void)
{
  int is_fcgi_init = 0;

  while (FCGI_Accept() >= 0)
  {
    if ( is_fcgi_init == 0 )
    {
      init_shared_memory();
      is_fcgi_init = 1;
    }
    
    local_call_count++;
    fprintf(stderr, "[bcc.fcgi] called, local_call_count = %d\n", local_call_count);
    fflush(stderr);

    // Self-Healing Check
    if (config->layout_version != LAYOUT_VERSION)
    {
      fprintf(stderr, "[bcc.fcgi] PID %d detected updated SHM layout. Re-mapping...\n", getpid());
      fflush(stderr);
      init_shared_memory();
    }

    char *method = getenv("REQUEST_METHOD");
    const char *mode;
    init_query();
    parse_query();
    mode = get_query_val("mode");

    // --- 1. MODE: CONFIG UPDATE (Writer) ---
    if (mode && strcmp(mode, "update") == 0 && method && strcmp(method, "POST") == 0)
    {
      int len = atoi(getenv("CONTENT_LENGTH") ? getenv("CONTENT_LENGTH") : "0");
      char *upload_buffer = malloc(len + 1);
      if (upload_buffer)
      {
        fread(upload_buffer, 1, len, stdin);
        upload_buffer[len] = '\0';

        // Sync to disk
        FILE *f = fopen(CONFIG_PATH, "w");
        if (f)
        {
          fwrite(upload_buffer, 1, len, f);
          fclose(f);
          
          // Trigger double-buffered swap
          read_config_data_from_disk();
          __sync_fetch_and_add(&config->update_count, 1);
          
          fprintf(stderr, "[bcc.fcgi] Update to %s successful, content-length=%d\n", CONFIG_PATH, len);
        }
        else
        {
          fprintf(stderr, "[bcc.fcgi] Failed to open %s for writing\n", CONFIG_PATH);
        }
        free(upload_buffer);
      }

      fflush(stderr);
      int active = config->active_buffer;
      printf("Content-type: application/json\r\n\r\n%s", (active >= 0) ? config->json_data[active] : "[]");
      fflush(stdout);
    }


    // --- 2. MODE: TASK EXECUTION (Reader) ---
    else if (mode && strcmp(mode, "task") == 0 && method && strcmp(method, "POST") == 0)
    {
      // the task process should check if any active buffer is available, if not then it should also call "read_config_data_from_disk"
      if (config->active_buffer < 0)
      {
        read_config_data_from_disk();
      }
      
      __sync_fetch_and_add(&config->task_count, 1);

      int len = atoi(getenv("CONTENT_LENGTH") ? getenv("CONTENT_LENGTH") : "0");
      char *task_data = (char *) malloc(len + 1);
      if (task_data)
      {
        size_t read_len = fread(task_data, 1, len, stdin);
        task_data[read_len] = '\0';

        fprintf(stderr, "[bcc.fcgi] Task data read, content-length=%d\n", len);
        fflush(stderr);

        printf("Content-type: application/json\r\n\r\n");
        int active = config->active_buffer;
        if (active >= 0)
          bcc_task(config->json_data[active], task_data);
        else
          printf("[]"); // Fallback
          
        fflush(stdout);
        free(task_data);
      }
      else
      {
        printf("Status: 500 Internal Server Error\r\nContent-type: text/plain\r\n\r\nOut of memory\n");
        fflush(stdout);
      }
    }
    
    // --- 3. DASHBOARD (Observer) ---
    else
    {
      if (config->active_buffer < 0)
      {
        read_config_data_from_disk();
      }
      
      int active = config->active_buffer;
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

      printf("<h1>FastCGI BCC Node Monitor <span class='status-tag'>Active</span></h1>");

      printf("<table>"
             "<tr><th>Metric Group</th><th>Field</th><th>Value</th></tr>"
             "<tr><td><b>Version Control</b></td><td>Layout Version</td><td><code>%d</code></td></tr>"
             "<tr><td><b>Worker Process</b></td><td>Current PID</td><td>%d</td></tr>"
             "<tr><td></td><td>Calls to this PID</td><td>%d</td></tr>"
             "<tr><td></td><td>Heap Usage</td><td>%lu KB</td></tr>"
             "<tr><td><b>Global Activity</b></td><td>Total Updates</td><td>%lu</td></tr>"
             "<tr><td></td><td>Total Tasks</td><td>%lu</td></tr>"
             "<tr><td></td><td>Disk Reloads</td><td>%lu</td></tr>"
             "<tr><td><b>Configuration</b></td><td>Active Buffer</td><td>%d</td></tr>"
             "<tr><td></td><td>Config Length</td><td>%d bytes</td></tr>"
             "</table>",
             config->layout_version, getpid(), local_call_count, get_heap_usage(), config->update_count, config->task_count, config->disk_read_count, active, (active >= 0) ? config->config_len[active] : 0);

      printf("<h3>Global RAM Configuration (Buffer %d):</h3><pre>%s</pre>", active, (active >= 0) ? config->json_data[active] : "N/A");
      printf("</div></body></html>");
      fflush(stdout);
    }
    
    free_query();
  }

  return 0;
}
