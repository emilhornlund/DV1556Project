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

	std::string userCommand, commandArr[MAXCOMMANDS];
	std::string user = "user";    // Change this if you want another user to be displayed
	std::string currentDir;    // current directory, used for output

    bool bRun = true;

    /* TEST */
    /*std::string garbage;
    fileSystem.createFile("ab", user, garbage, true);
    fileSystem.createFile("bb", user, garbage, true);
    std::string fileContent = "FnKMB2eueh1c3jEytgraSHCsCqlSYKVRa8srQdIZqrB6LJNcVB9Az60zd6b9yMCKTeHK2AoHZtjaObE95L99Mpr8sSNsCMIuDu6YGIsQBVPqZoe9E0PK9NSdZuZo1pYdBHorxhEPanNdWWCJQqTROswtwYa6ybS52k346xXgoPSIDWuh37mIyg5ezoTx5LEuDT6sS4uBe3DRS0pIwQ6mYlYPxg4huyvsIWIGAtEeDu1lNrVB0ZQws6UK7LeOfl3rgvpUPgsHaR7ukrLT5i75E20130oovSJemwfT1x1DLvUhscj5HvUmeiW6VR6yjyIWOLtELmptivB3hySY9jrba4UbK5Na3dgewShCqii7r1x5Mjzj4LVawDB6J4M1Qahmvj4agI9ZgNElrOtk0QZhxFnJDfeUqMKzV94phBME4ct2X2XdGwQvUexVfIdmfwHMVcnWgXnejoBRuaFxvVIva1UvZRuZ5qW6WPqtM30XjOdle3wic5JbsNzGqjE7L2g9XSF3Lc7Mp7iKry43J27uCEvKZC4VHQbjmZoOYf0pLTJW7NilnuEhNlyYU6YWB5hgM2ewEXflWCZJc09tt4m5AEB0U3y3NOMw3un64r95THYGhxpuYXLpfdgVhHIoMIiWHefH6Bq1mFE0Ufim6OG525JI96ZHZarj48FpC8jOOTEdjGvSCIb9a3Fc7JfRMpmaEwd5DfQnEpHSCjQBZOepEGsIKDmobpFPAy0NZZmnxnKzskgRQCGuCtMJaTnzwrvNiPafqTS2a418SIKkfwyTVtRGyiFmeIElVrkiQroXbsznGlq0VCZLWn8hTXeAVDB30jdsBe57tmQ4QIJ9LjSkcCqBX1JvlLIuSFXuV9m2UIx31jnXqUJlWcoZ5LWI1qAzxjXZkG09TNWu8N2CoRWjNxUOI2RxIRwa6gEutP8wZYBCUybMvPzFkai1i2r68mu0zQoHSm3YeGfvtVjJtpRpduwlaZhkkNwEn4hATDYOltbqYt9zNCiyY2ULQD91iUkrLIyqzIFALnEkkyhiarlK0KqCnua9HmUeO7GoUrzTlL4foBm7RvZo943LYxxYp9KPooaqlyhWmUG1KHsQG0WVPlINXkatvcoGvpCH3cOApPm2WS9huzYAlVhoxpXC2zN7vGELKfWgM86q0Zkp3fdn1lLIMbB4hG18WawXGAwq0txOfBJIJcLvODfYk6xPbCdRxWF7a7lK7Vjf5xKlydsPckA526iEQYPa7hZHjLE8g9eAtZbyJn6DWH7nUFSMGNYzNbsP09StTCv1NxiQi80B";
    fileSystem.createFile("b.txt", user, fileContent, false);



    std::string contentA = "hejmittnamnärA";
    std::string contentB = "sugen?";
    fileSystem.createFile("a.txt", user, contentA, false);


    fileSystem.appendFile("a.txt", "b.txt");
    fileSystem.changePermission("4", "b");*/
    fileSystem.restoreFilesystem("filesystem.fs");

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
    helpStr += "* chmod  <permission> <filepath>:   Changes permission on a file or folder\n";
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
        std::string garbage;
        try {
            fileSystem.createFile(filepath, username, garbage, true);
        } catch (const char* e) {
            std::cout << e << std::endl;
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
            std::cout << e << std::endl;
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
            std::cout << e << std::endl;
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
            std::cout << e << std::endl;
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
            std::cout << e << std::endl;
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
            std::cout << e << std::endl;
        }
    } else {
        std::cout << "Usage: cp <filepath> <filepath>" << std::endl;
    }
}

void append(FileSystem &fileSystem, std::string *strArr, int nrOfCommands) {
    if (nrOfCommands > 2) {
        std::string fromFilepath = strArr[1];
        std::string toFilepath = strArr[2];
        try {
            fileSystem.appendFile(fromFilepath, toFilepath);
        } catch (const char *e) {
            std::cout << e << std::endl;
        }
    } else {
        std::cout << "Usage: append <filepath> <filepath>" << std::endl;
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
