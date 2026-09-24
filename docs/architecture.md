# ProcessPilot Architecture

```text
processpilot.conf
       |
       v
ProcessSupervisor
 |       |       |
pgrep    ps    restart
 |       |       |
 +-------+-------+
         |
     status/recovery
```

The project can later be extended with systemd/D-Bus integration,
structured logging, restart backoff, watchdogs, and metrics export.
