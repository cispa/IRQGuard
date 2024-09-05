## Instructions

First, compile and load the IRQGuard kernel module.
Then, compile the code using `make` then run the script `./plot.sh <glyph>`, e.g., `./plot.sh a`, to execute the toilet application on its own.
To simulate an attack, execute `./attack.sh <glyph>`. In this case, the program should be aborted by IRQGuard.
