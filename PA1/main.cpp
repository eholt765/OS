/*
 * Copyright (c) 2022, Justin Bradley
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <fcntl.h> 

#include "command.hpp"
#include "parser.hpp"

#define MAX_ALLOWED_LINES 25
 
 void execture(std::vector<shell_command> shell_commands){

    int execute = 1;
    /* Rule 1: Dont forget this should not go inside a while(1) loop anywhere in your program */
    for (shell_command sc: shell_commands) {
        if (execute) {
        pid_t pid;
        /*fork a child process*/
        pid = fork();
        if (pid < 0){ /* Rule 2: Exit if Fork failed */
            fprintf(stderr, "Fork Failed");
            exit(1);
        }
        else if (pid == 0) { //child process

            //OUTPUT REDIRECTIONS options are term(normal), file, append, pipe(PA2)

            //check for output redirection using enum class ostream_mode
            ostream_mode out_redr = sc.cout_mode;
            if(out_redr == ostream_mode::file){
                //files name
                std::string output_file = sc.cout_file;

                int file = open(output_file.c_str(), O_WRONLY | O_CREAT , 0444); // O_CREAT requires that 3rd arg for open be used code wiki, stackoverflow for what to put for 3rd argument and code wiki for 2nd args
                
                dup2(file, 1); //set standard output to file
            }

            else if(out_redr == ostream_mode::append){
                //files name
                std::string output_file = sc.cout_file;

                int file = open(output_file.c_str(), O_APPEND); //for append only output
                
                dup2(file, 1); //set standard output to append
            }

            //INPUT REDIRECTIONS option are term(normal), file, and pipe (PA2)

            //check if input redirected using enum class istream_mode
            istream_mode in_redr = sc.cin_mode;
            if(in_redr == istream_mode::file){
                // files name
                std::string input_file = sc.cin_file;
                
                int file = open(input_file.c_str(), O_RDONLY); //for input so open as read only

                dup2(file, 0); //2nd arg 0 is for standard input
            }

            // EXECUTE COMMAND 

            if (execute){
                //converts sc.args into correct input for 2nd execvp argument used infromation on stackoverflow to get args into correct format
                std::vector<char*> convert_args = {const_cast<char*>(sc.cmd.c_str())};
                for (const auto& a : sc.args) {
                    convert_args.emplace_back(const_cast<char*>(a.c_str()));
                }
                convert_args.emplace_back(nullptr);

                execvp(sc.cmd.c_str(), convert_args.data());
                exit(1); /*Rule 3: Always exit in child */
            }
        }
        else { //parent process
            //parent will wait for child to complete
            int status;
            wait(&status); /* Rule 4: Wait for child unless we need to launch another exec */

            //modes on_success, on_fail, always(normal)
            next_command_mode next_cmd_mode = sc.next_mode;

            //must have previous success
            if(next_cmd_mode == next_command_mode::on_success){
                if(!WEXITSTATUS(status)){ //used info on tutorialspoint to select WEXITSTATUS
                  //  printf("command passed\n");
                    execute = 1;
                }
                else {
                  //  printf("command failed\n");
                    execute = 0;
                }
            }

            //must have previous fail
            if(next_cmd_mode == next_command_mode::on_fail){
                if(WEXITSTATUS(status)){
                 //   printf("command failed\n");
                    execute = 1;
                }
                else {
                 //   printf("command passed\n");
                    execute = 0;
                }
            }

        }
      }
    }
}


int main(int argc, char* argv[])
{
    std::string input_line;

    std::vector<shell_command> shell_commands;

    if (argc > 1 && argv[1] == std::string("-t")) { // -t option

      while (std::getline(std::cin, input_line)) {
          if(input_line == std::string("exit")) {

              exit(0);
          }
          else {
              try {
                  // Parse the input line.
                  shell_commands = parse_command_string(input_line);

                  execture(shell_commands);
              }
              catch (const std::runtime_error& e) {
                  std::cout << e.what() << "\n";
              }
          }
      }
    }
    else {
      for (int i=0;i<MAX_ALLOWED_LINES;i++) { // Limits the shell to MAX_ALLOWED_LINES
        // Print the prompt.
        std::cout << "osh> " << std::flush;

        // Read a single line.
        if (!std::getline(std::cin, input_line) || input_line == "exit") {
            break;
        }

        try {
            // Parse the input line.
            shell_commands = parse_command_string(input_line);

            execture(shell_commands);
        }
        catch (const std::runtime_error& e) {
            std::cout << "osh: " << e.what() << "\n";
        }
    }
  }
  
    std::cout << std::endl;
}