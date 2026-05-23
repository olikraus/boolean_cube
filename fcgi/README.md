# Overview

`fcgi_bcc` is a fast-CGI wrapper for `bcc`. 
The following calls are supported:

 * GET No URL arguments: Show status web page.
 * POST mode=task: Read bcc json and generate result
 * POST mode=update: Read bcc (config) json and reuse this json for all tasks as prefix


# URL Syntax

Base URL (example):
```
http://localhost/bcc.fcgi
```

POST URL for updating the shared config prefix:
```
http://localhost/bcc.fcgi?mode=update
```

POST URL for executing a task (config prefix + task payload):
```
http://localhost/bcc.fcgi?mode=task
```

Example POST call for `mode=update`:
```
curl -v -H "Content-Type: application/json" --data @../json/intersection_xgroups.json "http://localhost/bcc.fcgi?mode=update"
```

Example POST call for `mode=task`:
```
curl -v -H "Content-Type: application/json" --data @../json/intersection_plain.json "http://localhost/bcc.fcgi?mode=task"
```

Notes:
 - `mode=update` stores the uploaded JSON as shared prefix/config.
 - `mode=task` executes with the current shared prefix and the uploaded task JSON.
 - No `mode` (plain GET) shows the status web page.


# Implementation


The mode=update uses a double buffer swap method with atomic switch.
Drawback is, that there must be a little bit of time between mode=update
commands: 5 Seconds should be fine.

The mode=update content will be stored in a shared memory area (for quick)
access and inside `/var/lib/` so that the configuration json will survive a system reset.


The JSON, which is uploaded to fcgi_bcc is stored under
```
/var/lib/fcgi_bcc/config.json 
```
It is required to create a suitable folder:
```
sudo mkdir -p /var/lib/fcgi_bcc
sudo chmod a+rw /var/lib/fcgi_bcc
```


# Build

Requires the definition of `CO_FCGI` so that c-object uses the redefined `FILE` and stdio macros.
Also execute a `make clean` to ensure that all files are recompiled.


# VS Code Development Scripts (Windows/MSYS2)

The following PowerShell scripts are intended for VS Code development and testing only.
They are not part of production deployment on the target web server.

 - `../bcc/run_bcselftest_msys2.ps1`
	 - Builds the command line `bcc` binary (with selectable `BC_EXT`) and runs `-test` selftests.
	 - Example:
```
pwsh ../bcc/run_bcselftest_msys2.ps1 -Clean -BcExt 2
```

 - `build_fcgi_msys2.ps1`
	 - Builds the `bcc.fcgi` executable in this folder (with selectable `BC_EXT`).
	 - Example:
```
pwsh ./build_fcgi_msys2.ps1 -Clean -BcExt 2
```


# Deploy

## Ubuntu, Apache2
```
sudo cp bcc.fcgi /var/www/html/.
```
or
```
sudo make ubuntu-install
```


# Cleanup

Kill all existing bcc.fcgi processes:
```
sudo pkill bcc.fcgi
```
Clear the shared memory segment to reset the stuck lock:
```
sudo rm /dev/shm/fcgi_bcc_config_ram
```
Restart Apache2
```
sudo systemctl restart apache2
```

