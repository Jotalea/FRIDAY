# FRIDAY
AI assistant for command line Linux, successor to Jarvis

~~enshittify~~ **improve** your Linux experience with FRIDAY, the AI assistant from the Marvel Cinematic Universe, who will help you with basic tasks on your Linux system [1].

## use words instead of commands
with FRIDAY you can directly tell the machine what to do, no need for writing long, annoying commands.

## use [2]
```bash
~ $ friday make a new file in ~/friday/src/ named patch1.cpp and open it with the best text editor

Suggested command:
mkdir -p friday/src && touch friday/src/patch1.cpp && vim friday/src/patch1.cpp
Explanation: Creates the directory if it doesn't exist, creates the file, and opens it with vim.

Run? [y/N]:
```
```bash
~ $ friday run neofetch from the git repo

Suggested command:
git clone https://github.com/dylanaraps/neofetch.git && cd neofetch && chmod +x neofetch && ./neofetch
Explanation: Clones the neofetch repository, navigates to it, makes it executable, and runs it.

Run? [y/N]:
```

## source
the source is available through git on https://github.com/Jotalea/FRIDAY.git [3]

## dependencies
requires libcurl and json.hpp (included)

## notes
[1] FRIDAY may work on non-Linux systems, but that is not its intended purpose
[2] by default, the execution of the suggested command is set to "Cancel", so you can avoid accidentally allowing it to run unwanted scripts
[3] licensed under the MIT License
