## Lunix-TNG : Character Device Driver

### Configuration & Execution

* Run make
* This produces the lunix.ko file to be installed in kernel
* and also the testMultipleRead executable to test the driver
* Usage: ./testMultipleRead /dev/lunix0-batt 1 (for cooked data, 0 for raw)
