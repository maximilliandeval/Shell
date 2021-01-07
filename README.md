# Shell

This is a bash-inspired shell application that offers multiprocessing support.  When you type in a command (in response to the shell prompt), the shell creates a child process that executes the command that was entered.  The shell has 3 built-in commands: 

1. `jobs`: print the list of processes that are currently running in the background
2. `kill <PID>`: kill the child process with the specified PID (if it exists)
3. `exit`: quit the shell

When a non-built-in command is entered, the shell creates a new child process which invokes the specified executable program.  These processes can be specified to run in the foreground or in the background.
Non-built-in commands should contain the full path to the executable file (e.g., `/bin/ls` rather than just `ls`) and may contain an arbitrary number of command-line options (e.g., `/bin/ls -l -r`).
To indicate that a command should be executed as a background process, include the ampersand (`&`) character at the end of the command.  For example:
`/bin/sleep 10 &`.

Every time the shell receives a new command, it scans the list of background processes.  If any of these processes have completed, it updates the list and prints: `<PID> finished with exit code <STATU>S`.



To compile the shell, run: `make`
To start the shell, run: `./shell`
