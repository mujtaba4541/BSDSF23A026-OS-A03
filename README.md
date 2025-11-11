# OS Assignment 03 - Custom Shell

A custom UNIX shell implementation for Operating Systems course.

## Features Implemented

1. **Feature 1** - Base Shell (v1.0-base)
   - Basic command execution
   - Command parsing and tokenization
   - Process management

2. **Feature 2** - Built-in Commands (v2.0-builtins)
   - cd, exit, help, jobs commands
   - Internal command execution without forking

3. **Feature 3** - Command History (v3.0-history)
   - history command
   - !n command to re-execute history
   - Stores last 20 commands

4. **Feature 4** - Readline Integration (v4.0-readline)
   - Tab completion
   - History navigation with arrow keys
   - Line editing features

5. **Feature 5** - I/O Redirection and Pipes (v5.0-redirection)
   - Input redirection (<)
   - Output redirection (>)
   - Pipes (|) between commands

6. **Feature 6** - Command Chaining and Background Execution (v6.0-multitasking)
   - Command chaining (;)
   - Background execution (&)
   - jobs command for background job management

7. **Feature 7** - If-then-else-fi Control Structure (v7.0-control)
   - Conditional execution
   - Multiline block reading
   - Exit status based branching

8. **Feature 8** - Shell Variables (v8.0-variables)
   - Variable assignment (VAR=value)
   - Variable expansion ($VAR)
   - set command to display variables

## Building

```bash
make
