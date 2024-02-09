# Shell

Unix shell written in C

## Group Number
Group 38

## Group Name
Group 38

## Group Members
- **Kobe Myers**: ktm22c@fsu.edu
- **Jack Throdahl** jtt20q@fsu.edu
- **Osher Steel** os19h@fsu.edu
## Division of Labor

### Part 1: Prompt
- **Responsibilities**: [Description]
- **Assigned to**: Jack, Osher

### Part 2: Environment Variables
- **Responsibilities**: [Description]
- **Assigned to**: Kobe, Osh

### Part 3: Tilde Expansion
- **Responsibilities**: [Description]
- **Assigned to**: Kobe, Jack

### Part 4: $PATH Search
- **Responsibilities**: [Description]
- **Assigned to**: Osher, Kobe

### Part 5: External Command Execution
- **Responsibilities**: [Description]
- **Assigned to**: Jack, Osher

### Part 6: I/O Redirection
- **Responsibilities**: [Description]
- **Assigned to**: Kobe, Jack

### Part 7: Piping
- **Responsibilities**: [Description]
- **Assigned to**: Osher, Kobe

### Part 8: Background Processing
- **Responsibilities**: [Description]
- **Assigned to**: Jack, Osher

### Part 9: Internal Command Execution
- **Responsibilities**: [Description]
- **Assigned to**: Kobe, Jack

### Extra Credit
- **Responsibilities**: [Description]
- **Assigned to**: Kobe, Osher, Jack

## File Listing
```
shell/
│
├── src/
│ ├── main.c
│ └── lexer.c
│
├── include/
│ └── lexer.h
│
├── README.md
└── Makefile
```
## How to Compile & Execute

### Requirements
- **Compiler**: gcc

### Compilation
```bash
make
```
This will build the executable in ...
### Execution
```bash
bin/shell
```
This will run the program ...

## Bugs

- The function setUpPipes can take in any number of commands and runs a loop to create the child processes but the piping does not seem to work for more than 2 commands
