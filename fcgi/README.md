# Overview

`fcgi_bcc` is a fast-CGI wrapper for `bcc`. 
The following calls are supported:

 * GET No URL arguments: Show status web page.
 * POST mode=task: Read bcc json and generate result
 * POST mode=update: Read bcc json and reuse this json for all tasks as prefix
 
# Deploy

## Ubuntu, Apache2
```
sudo cp bcc.fcgi /var/www/html/.
```


