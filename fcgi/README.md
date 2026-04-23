# Overview

`fcgi_bcc` is a fast-CGI wrapper for `bcc`. 
The following calls are supported:

 * GET No URL arguments: Show status web page.
 * POST mode=task: Read bcc json and generate result
 * POST mode=update: Read bcc (config) json and reuse this json for all tasks as prefix


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

