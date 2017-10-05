#include <iostream>
#include <sstream>
#include "filesystem.h"

const int MAXCOMMANDS = 8;
const int NUMAVAILABLECOMMANDS = 15;

std::string availableCommands[NUMAVAILABLECOMMANDS] = {
    "quit","format","ls","create","cat","createImage","restoreImage",
    "rm","cp","append","mv","mkdir","cd","pwd","help"
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

int main(void) {
    FileSystem fileSystem;

	std::string userCommand, commandArr[MAXCOMMANDS];
	std::string user = "user";    // Change this if you want another user to be displayed
	std::string currentDir;    // current directory, used for output

    bool bRun = true;

    /* TEST */
    fileSystem.createFile("a", user, true);
    fileSystem.createFile("b", user, true);
    fileSystem.createFile("a/c.txt", user, false);
    fileSystem.copy("a/c.txt", "b/c.txt");

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
                break;
            case 6: // restoreImage
                break;
            case 7: // rm
                rm(fileSystem, commandArr, nrOfCommands);
                break;
            case 8: // cp
                cp(fileSystem, commandArr, nrOfCommands);
                break;
            case 9: // append
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
            default:
                std::cout << "Unknown command: " << commandArr[0] << std::endl;
            }
        }
    } while (bRun == true);

    return 0;
}

int parseCommandString(const std::string &userCommand, std::string strArr[]) {
    std::stringstream ssin(userCommand);
    int counter = 0;
    while (ssin.good() && counter < MAXCOMMANDS) {
        ssin >> strArr[counter];
        counter++;
    }
    if (strArr[0] == "") {
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
    helpStr += "* mv     <old-file> <new-file>:     Renames <old-file> to <new-file>\n";
    helpStr += "* mkdir  <directory>:               Creates a new directory called <directory>\n";
    helpStr += "* cd     <directory>:               Changes current working directory to <directory>\n";
    helpStr += "* pwd:                              Get current working directory\n";
    helpStr += "* help:                             Prints this help screen\n";
    return helpStr;
}

/* Insert code for your shell functions and call them from the switch-case */
void ls(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 1) {
        try {
            std::string filepath = strArr[1];
            std::cout << fileSystem.listDir(filepath) << std::endl;
        } catch (const char* e) {
            std::cout << e << std::endl;
        }

    } else {
        std::cout << fileSystem.listDir() << std::endl;
    }
}

void mkdir(FileSystem &fileSystem, std::string *strArr, int nrOfCommands, std::string username) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];
        fileSystem.createFile(filepath, username, true);
    }
}

void cd(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];
        int retVal = fileSystem.moveToFolder(filepath);
    } else {
        int retVal = fileSystem.moveToFolder();
    }
}

void cat(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];
        try {
            std::string content = fileSystem.cat(filepath);
            std::cout << content << std::endl;
        } catch (const char* e) {
            std::cout << e << std::endl;
        }
    }
}

void create(FileSystem &fileSystem, std::string *strArr, int nrOfCommands, const std::string &username) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];
        try {
            fileSystem.createFile(filepath, username, false);
        }
        catch (const char *e) {
            std::cout << e << std::endl;
        }
    }
}

void rm(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 1) {
        std::string filepath = strArr[1];
        try {
            fileSystem.removeFile(filepath);
        } catch (const char *e) {
            std::cout << e << std::endl;
        }
    }
}

void mv(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 2) {
        std::string fromFilepath = strArr[1];
        std::string toFilepath = strArr[2];
        try {
            fileSystem.move(fromFilepath, toFilepath);
        } catch (const char *e) {
            std::cout << e << std::endl;
        }
    }
}

void cp(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 2) {
        std::string fromFilepath = strArr[1];
        std::string toFilepath = strArr[2];
        try {
            fileSystem.copy(fromFilepath, toFilepath);
        } catch (const char *e) {
            std::cout << e << std::endl;
        }
    }
}