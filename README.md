# loadgen

CPU, network and disk load generation and log collection scripts.

## How to Use

1. Add the VMs IPs in controller machine's `/etc/hosts`
2. `script.all-load.sh` ssh'es to specified PMs and VMs, starts off load, starts off logging, stops logging, stops load, collects logs to specified repo\_IP
3. Copy the scripts of `dom0-scripts` to the dom0 where logging is to be done. Execute `make` 
4. Copy the scripts of `domU-scripts` to the domU where logging is to be done. Execute `make`
5. Copy the scripts of `controller-scripts` on one controller machine 
6. Will need ssh key exchange between all these machines, so that the script can do all the ssh without manual intervention at password prompt

