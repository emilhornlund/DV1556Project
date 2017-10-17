#include <iostream>
#include <sstream>
#include "filesystem.h"
#include <fstream>

const int MAXCOMMANDS = 8;
const int NUMAVAILABLECOMMANDS = 16;

std::string availableCommands[NUMAVAILABLECOMMANDS] = {
    "quit","format","ls","create","cat","createImage","restoreImage",
    "rm","cp","append","mv","mkdir","cd","pwd","help", "chmod"
};

/* Takes usercommand from input and returns number of commands, commands are stored in strArr[] */
int parseCommandString(const std::string &userCommand, std::string strArr[]);
int findCommand(std::string &command);
bool quit();
std::string help();

/* More functions ... */
void ls(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);
void mkdir(FileSystem &fileSystem, std::string *strArr, int nrOfCommands, std::string username);
void cd(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);
void cat(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);
void create(FileSystem &fileSystem, std::string *strArr, int nrOfCommands, const std::string &username);
void rm(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);
void mv(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);
void cp(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);
void append(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);
void chmod(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);
void saveFilesystem(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);
void restoreFilesystem(FileSystem &fileSystem, std::string *strArr, int nrOfCommands);

int main(void) {
    FileSystem fileSystem;

	std::string userCommand, commandArr[MAXCOMMANDS] = {""};
	std::string user = "user";    // Change this if you want another user to be displayed
	std::string currentDir;    // current directory, used for output

    bool bRun = true;

    do {
        currentDir = fileSystem.getPWD();
        std::cout << user << "@DV1492:" << currentDir << "$ ";
        getline(std::cin, userCommand);
        int nrOfCommands = parseCommandString(userCommand, commandArr);
        if (nrOfCommands > 0) {
            int cIndex = findCommand(commandArr[0]);
            switch(cIndex) {

			case 0: //quit
				bRun = quit();                
                break;
            case 1: // format
                fileSystem.format();
                break;
            case 2: // ls
                    ls(fileSystem, commandArr, nrOfCommands);
                break;
            case 3: // create
                create(fileSystem, commandArr, nrOfCommands, user);
                break;
            case 4: // cat
                cat(fileSystem, commandArr, nrOfCommands);
                break;
            case 5: // createImage
                saveFilesystem(fileSystem, commandArr, nrOfCommands);
                break;
            case 6: // restoreImage
                restoreFilesystem(fileSystem, commandArr, nrOfCommands);
                break;
            case 7: // rm
                rm(fileSystem, commandArr, nrOfCommands);
                break;
            case 8: // cp
                cp(fileSystem, commandArr, nrOfCommands);
                break;
            case 9: // append
                append(fileSystem, commandArr, nrOfCommands);
                break;
            case 10: // mv
                mv(fileSystem, commandArr, nrOfCommands);
                break;
            case 11: // mkdir
                mkdir(fileSystem, commandArr, nrOfCommands, user);
            break;
            case 12: // cd
                cd(fileSystem, commandArr, nrOfCommands);
                break;
            case 13: // pwd
                std::cout << fileSystem.getPWD() << std::endl;
                break;
            case 14: // help
                std::cout << help() << std::endl;
                break;
            case 15: //chmod
                chmod(fileSystem, commandArr, nrOfCommands);
                break;
            default: {
                if (nrOfCommands == 3 && commandArr[1] == ">") {
                    commandArr[1] = commandArr[0];
                    append(fileSystem, commandArr, nrOfCommands);

                } else {
                    std::cout << "Unknown command: " << commandArr[0] << std::endl;
                }
            } break;
            }
        } else {
            std::cout << "Enter command - see help" << std::endl;
        }
    } while (bRun);

    return 0;
}

int parseCommandString(const std::string &userCommand, std::string strArr[]) {
    std::stringstream ssin(userCommand);
    int counter = 0;
    while (ssin.good() && counter < MAXCOMMANDS) {
        ssin >> strArr[counter];
        counter++;
    }
    if (userCommand.empty()) {
        counter = 0;
    }
    return counter;
}
int findCommand(std::string &command) {
    int index = -1;
    for (int i = 0; i < NUMAVAILABLECOMMANDS && index == -1; ++i) {
        if (command == availableCommands[i]) {
            index = i;
        }
    }
    return index;
}

bool quit() {
	std::cout << "Exiting\n";
	return false;
}

std::string help() {
    std::string helpStr;
    helpStr += "OSD Disk Tool .oO Help Screen Oo.\n";
    helpStr += "-----------------------------------------------------------------------------------\n" ;
    helpStr += "* quit:                             Quit OSD Disk Tool\n";
    helpStr += "* format;                           Formats disk\n";
    helpStr += "* ls     <path>:                    Lists contents of <path>.\n";
    helpStr += "* create <path>:                    Creates a file and stores contents in <path>\n";
    helpStr += "* cat    <path>:                    Dumps contents of <file>.\n";
    helpStr += "* createImage  <real-file>:         Saves disk to <real-file>\n";
    helpStr += "* restoreImage <real-file>:         Reads <real-file> onto disk\n";
    helpStr += "* rm     <file>:                    Removes <file>\n";
    helpStr += "* cp     <source> <destination>:    Copy <source> to <destination>\n";
    helpStr += "* append <source> <destination>:    Appends contents of <source> to <destination>\n";
    helpStr += "* <source> > <destination>:         Appends contents of <source> to <destination>\n";
    helpStr += "* mv     <old-file> <new-file>:     Renames <old-file> to <new-file>\n";
    helpStr += "* mkdir  <directory>:               Creates a new directory called <directory>\n";
    helpStr += "* cd     <directory>:               Changes current working directory to <directory>\n";
    helpStr += "* pwd:                              Get current working directory\n";
    helpStr += "* chmod  <permission> <filepath>:   Changes permission on a file or folder\n";
    helpStr += "* help:                             Prints this help screen\n";
    return helpStr;
}

/* Insert code for your shell functions and call them from the switch-case */
void ls(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 1) {
        std::string filepath;
        try {
            filepath = strArr[1];
            std::cout << fileSystem.listDir(filepath) << std::endl;
        } catch (const char* e) {
            std::cout << "ls: " << filepath << ": " << e << std::endl;
        }

    } else {
        std::string empty = "";
        std::cout << fileSystem.listDir(empty) << std::endl;
    }
}

void mkdir(FileSystem &fileSystem, std::string *strArr, int nrOfCommands, std::string username) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];
        std::string garbage;
        try {
            fileSystem.createFile(filepath, username, garbage, true);
        } catch (const char* e) {
            std::cout << "mkdir: " << filepath << ": " << e << std::endl;
        }
    } else {
        std::cout << "Usage: mkdir <folder/folderpath>" << std::endl;
    }
}

void cd(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];
        try {
            fileSystem.moveToFolder(filepath);
        } catch (const char* e) {
            std::cout << "cd: " << filepath << ": " << e << std::endl;
        }
    } else {
        try {
            fileSystem.moveToFolder();
        } catch (const char* e) {
            std::cout << e << std::endl;
        }
    }
}

void cat(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];
        try {
            std::string content = fileSystem.cat(filepath);
            std::cout << content << std::endl;
        } catch (const char* e) {
            std::cout << "cat: " << filepath << ": " << e << std::endl;
        }
    } else {
        std::cout << "Usage: cat <filepath>" << std::endl;
    }
}

void create(FileSystem &fileSystem, std::string *strArr, int nrOfCommands, const std::string &username) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];

        std::string contents;
        std::cout << "Content: ";
        std::getline(std::cin, contents);

        try {
            fileSystem.createFile(filepath, username, contents, false);
        }
        catch (const char *e) {
            std::cout << e << std::endl;
        }
    } else {
        std::cout << "Usage: create <filepath>" << std::endl;
    }
}

void rm(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];
        try {
            fileSystem.removeFile(filepath);
        } catch (const char *e) {
            std::cout << "rm: " << filepath << ": " << e << std::endl;
        }
    } else {
        std::cout << "Usage: rm <folder/file -path>" << std::endl;
    }
}

void mv(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 2) {
        std::string fromFilepath = strArr[1];
        std::string toFilepath = strArr[2];
        try {
            fileSystem.move(fromFilepath, toFilepath);
        } catch (const char *e) {
            std::cout << "mv: " << fromFilepath << " to " << toFilepath << ": " << e << std::endl;
        }
    } else {
        std::cout << "Usage: mv <filepath> <filepath>" << std::endl;
    }
}

void cp(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 2) {
        std::string fromFilepath = strArr[1];
        std::string toFilepath = strArr[2];
        try {
            fileSystem.copy(fromFilepath, toFilepath);
        } catch (const char *e) {
            std::cout << "cp: " << fromFilepath << " to " << toFilepath << ": " << e << std::endl;
        }
    } else {
        std::cout << "Usage: cp <filepath> <filepath>" << std::endl;
    }
}

void append(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 2) {
        std::string fromFilepath = strArr[2];
        std::string toFilepath = strArr[1];
        try {
            fileSystem.appendFile(fromFilepath, toFilepath);
        } catch (const char *e) {
            std::cout << "append: " << fromFilepath << " to " << toFilepath << ": " << e << std::endl;
        }
    } else {
        std::cout << "Usage: append <filepath> <filepath> or <filepath> > <filepath>" << std::endl;
    }
}

void chmod(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 2) {
        std::string permission = strArr[1];
        std::string filepath = strArr[2];
        try {
            fileSystem.changePermission(permission, filepath);
        } catch (const char *e) {
            std::cout << e << std::endl;
        }
    } else {
        std::cout << "Usage: chmod <permission (1-7)> <filepath>" << std::endl;
    }
}

void saveFilesystem(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 1) {
        std::string path = strArr[1];
        try {
            fileSystem.saveFilesystem(path);
            std::cout << "Success: " << path << ".fs" << std::endl;
        } catch (const char* e) {
            std::cout << e << std::endl;
        }
    } else {
        std::cout << "Usage: createImage <path>" << std::endl;
    }
}

void restoreFilesystem(FileSystem &fileSystem, std::string *strArr, int nrOfCommands){
    if (nrOfCommands > 1) {
        std::string path = strArr[1];
        try {
            fileSystem.restoreFilesystem(path);
        } catch (const char* e) {
            std::cout << e << std::endl;
        }
    } else {
        std::cout << "Usage: restoreImage <path>" << std::endl;
    }
}
