# Shell

## Description
This is a POSIX shell implementation written in C. The shell implements core functionality of Bash, such as executing commands, piping, and redirection etc. A brief list of the supported features is given below:
- Command execution
- I/O redirection
- Piping
- Background processes
- Signal handling
- Wildcard expansion
- Command history
- Tab completion
- Chaining commands with `&&` and `||` and `;` operators
- Aliases
- Built-in commands

Additionaly, a comprehensive test suite is provided to test the shell's functionality.

## Usage
The project uses `readline` and `ncurses` libraries. To install these libraries, run the following commands:
```bash
sudo apt-get install libreadline-dev
sudo apt-get install libncurses5-dev
```

In order to build the project, run the following commands:
```bash
make
```

To run the shell, execute the following command:
```bash
build/Shell
```
Or you can also do:
```bash
make run
```

In order to run the tests, execute the following command:
```bash
make test
```