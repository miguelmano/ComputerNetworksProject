#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <fstream>

#define EMPTY '0'

using namespace std;

//**************************************
//********* String Functions ***********
//**************************************

char* turn_into_c_string(string str)
{
    char* c_str = new char [str.length()+1];
    strcpy(c_str, str.c_str());
    return c_str;
}

string turn_into_cpp_string(char* str, int n)
{
    string cpp_string = str;
    return cpp_string.substr(0, n);
}

vector<string> split_string(string str)
{
    vector<string> splitted;
    
    char* c_str = turn_into_c_string(str);

    char* c_token = strtok(c_str, " ");
    while (c_token != NULL)  
    {  
        string token = c_token;
        splitted.push_back(token);
        c_token = strtok(NULL, " \n");  
    }
    
    return splitted;
}

vector<string> first_n_tokens_of_string(string str, int n)
{
    vector<string> splitted;

    char* c_str = turn_into_c_string(str);

    char* c_token = strtok(c_str, " ");
    for (int i=0; i<n && c_token != NULL; i++)
    {  
        string token = c_token;
        splitted.push_back(token);
        c_token = strtok(NULL, " \n");  
    }

    return splitted;
}

char to_upper_case(char c)
{
    if (65 <= c && c <= 90) // c is upper case
        return c;
    else if (97 <= c && c <= 122) // c is lower case
        return toupper(c);
    else // c is not a letter
        return -1; 
}

string current_word_representation(vector<char> current_word)
{
    string res = "";
    for (int i=0; i < current_word.size(); i++)
    {
        if (current_word[i] == EMPTY)
        {
            res += "_ ";
        }
        else
        {
            res.push_back(current_word[i]);
            res += " ";
        }
    }
    res += "\n";
    return res;
}

//**************************************
//********** Error Functions ***********
//**************************************

void report_error_and_exit(string err_msg)
{
    cerr << err_msg << endl;
    exit(1);
}

bool PLID_is_valid(string PLID)
{
    if (PLID.size() != 6) // PLID must have 6 digits
        return false;

    if (PLID.find_first_of("0123456789") == PLID.npos) // c is not a digit
            return false;

    return true;
}

bool belongs_to_alphabet(char c)
{
    return (65 <= c && c <= 90) || (97 <= c && c <= 122);
}

bool is_a_letter(string letter)
{
    if (letter.size() != 1)
        return false;
    
    char c = letter[c];
    if (!belongs_to_alphabet(c))
        return false;

    return true;
}

bool is_valid_guess(string guess, int n_letters)
{
    if (guess.length() != n_letters)
        return false;
    
    for (char c : guess)
        if (!belongs_to_alphabet(c))
            return false;

    return true;
}

//**************************************
//********** Other Functions ***********
//**************************************

int get_case(string value)
{
         if (value == "start"      || value == "sg"  ) {return 0;}
    else if (value == "play"       || value == "pl"  ) {return 1;}
    else if (value == "guess"      || value == "gw"  ) {return 2;}
    else if (value == "scoreboard" || value == "sb"  ) {return 3;}
    else if (value == "hint"       || value == "h"   ) {return 4;}
    else if (value == "state"      || value == "st"  ) {return 5;}
    else if (value == "quit"       || value == "exit") {return 6;}
    else if (value == "rev"                          ) {return 7;}   
    else                                               {return 8;}
}

//**************************************
//**********  Connections  *************
//**************************************

string UDP(string GS_IP, string GS_port, string msg)
{
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    // open socket
    fd = socket(AF_INET, SOCK_DGRAM , 0); //UDP socket
    if(fd==-1)report_error_and_exit("Error: UDP: Failed to open socket.");

    // ...
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_DGRAM; //UDP socket
    
    // get server's address info
    errcode = getaddrinfo(GS_IP.c_str(), GS_port.c_str(), &hints, &res);
    if(errcode!=0)report_error_and_exit("Error: UDP: Failed to get server's address information.");
    
    // send message to server
    n = sendto(fd, turn_into_c_string(msg), msg.length(), 0, res->ai_addr, res->ai_addrlen);
    if(n==-1)report_error_and_exit("Error: UDP: Failed to send message to server.");
    
    // receive message from server
    addrlen = sizeof(addr);
    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if(n==-1)report_error_and_exit("Error: UDP: Failed to receive message from server.");
    write(1,"echo: ", 6); write(1, buffer, n);

    // close stuff
    freeaddrinfo(res);
    close(fd);

    string cpp_buffer = turn_into_cpp_string(buffer, n);
    return cpp_buffer;
}

string TCP_read_token_1_byte_at_a_time(int fd, bool & there_is_a_file)
{
    string res;
    char buffer[1];

    while (true)
    {
        int n_read = read(fd, buffer, 1);
        if (n_read == EOF) there_is_a_file = false;
        if (buffer[0] == ' ') break;
        res.append(buffer);
    }

    return res;
}

void TCP_read_Fdata_to_disk(int fd, string Fname, int Fsize)
{
    char buffer[128];
    ofstream file(Fname);
    int n_left = Fsize;
    while (n_left > 0)
    {
        memset(buffer, 0, 128);
        int n_read = read(fd, buffer, 128);
        if (n_read == -1) /*error*/ exit(1);
        else if (n_read == 0) break;
        file.write(buffer, n_read);
        n_left -= n_read;
    }
    file.close(); 
}

void TCP(string GS_IP, string GS_port, string msg, vector<string> & parsed_msg)
{
    int fd, errcode;
    ssize_t n;
    struct addrinfo hints, *res;

    // open socket
    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd==-1) report_error_and_exit("Error: TCP: Failed to open socket.");

    // ...
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    // get server's address info
    errcode = getaddrinfo(GS_IP.c_str(), GS_port.c_str(), &hints, &res);
    if (errcode!=0) report_error_and_exit("Error: TCP: Failed to get server's address information.");

    // establish connection
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n==-1) report_error_and_exit("Error: TCP: Failed to establish connection with server.");

    // send message to server
    char* c_msg = turn_into_c_string(msg);
    int n_bytes = msg.length();
    char* ptr = c_msg;
    int n_left = n_bytes;
    while (n_left > 0)
    {
        int n_written = write(fd, ptr, n_bytes);
        if (n_written <= 0) report_error_and_exit("Error: TCP: Failed to send part of the message to server.");
        n_left -= n_written;
        ptr += n_written;
    }

    // receive message from server
    bool there_is_a_file = true;
    
    // read msg_id
    parsed_msg.push_back(TCP_read_token_1_byte_at_a_time(fd, there_is_a_file));
    
    // read status
    parsed_msg.push_back(TCP_read_token_1_byte_at_a_time(fd, there_is_a_file));

    if (there_is_a_file)
    {
        // read Fname
        string Fname = TCP_read_token_1_byte_at_a_time(fd, there_is_a_file);
        parsed_msg.push_back(Fname);

        // read Fsize
        string Fsize_str = TCP_read_token_1_byte_at_a_time(fd, there_is_a_file);
        parsed_msg.push_back(Fsize_str);
        int Fsize = stoi(Fsize_str);

        // read Fdata to disk
        TCP_read_Fdata_to_disk(fd, Fname, Fsize);
    }

    // close stuff
    freeaddrinfo(res);
    close(fd);
}

//**************************************
//**********       Main       **********
//**************************************

int main(int argc, char** argv)
{
    // command line arguments reading: "./player [-n GS_IP] [-p GS_port]"
    string GS_IP;
    string GS_port;
    if (argc == 1) // default values
    {   
        GS_IP = "hello"; //TODO: IP DA NOSSA MAQUINA
        GS_port = "58094";
    }
    else if (argc == 3) // specified either [-n GS_IP] or [-p GS_port]
    {
        string flag = argv[1];
        if (flag == "-n")
        {
            GS_IP = argv[2];
            GS_port = "58094";
        }
        else if (flag == "-p")
        {
            GS_IP = "hello"; //TODO: IP DA NOSSA MAQUINA
            GS_port = argv[2];
        }
        else
        {
            report_error_and_exit("Error: you must follow the correct syntax: ./server <word_file> [-n GS_IP] [-p GS_port]");
        }
    }
    else if (argc == 5) // specified both [-n GS_IP] and [-p GS_port]
    {
        string flag_1 = argv[1];
        string flag_2 = argv[3];
        if (flag_1 == "-n" && flag_2 == "-p")
        {
            GS_IP = argv[2];
            GS_port = argv[4];
        }
        else if (flag_1 == "-p" && flag_2 == "-n")
        {
            GS_IP = argv[4];
            GS_port = argv[2];
        }
        else
        {
            report_error_and_exit("Error: you must follow the correct syntax: ./server <word_file> [-n GS_IP] [-p GS_port]");
        }
    }
    else
    {
        report_error_and_exit("Error: you must follow the correct syntax: ./server <word_file> [-n GS_IP] [-p GS_port]");
    }

    // player's stats
    vector<char> current_word; 
    string line, PLID;
    bool has_ongoing_game = false;
    int trials_count;
    int n_letters;
    int errors_count;
    int max_errors;
    
    // commands
    while(true)
    {
        // read player's input
        getline(cin, line);
        vector<string> parsed_line = split_string(line);
        
        bool exit = false;

        switch (get_case(parsed_line[0]))
        {
            case 0: // "<start or sg> <PLID>\n"
            {
                // error check
                if (parsed_line.size() != 2)
                {
                    cerr << "start: You must to follow the correct syntax: <start or sg> <PLID>" << endl;
                    break;
                }
                else if (!PLID_is_valid(parsed_line[1]))
                {
                    cerr << "start: <PLID> must have 6 digits." << endl;
                    break;
                }
                else if (has_ongoing_game)
                {
                    cerr << "start: You already have an ongoing game." << endl;
                    break;
                }

                // prepare msg to send: "SNG <PLID>\n"
                PLID = parsed_line[1];
                string msg = "SNG " + PLID + "\n";
                cout << "play: To send: " + msg;

                // send msg and receive reply from server: "RSG <status> [n_letters max_errors]\n"
                string recvd_msg = UDP(GS_IP, GS_port, msg);
                vector<string> parsed_msg = split_string(recvd_msg);

                // execute command
                string msg_id = parsed_msg[0];
                string status = parsed_msg[1];
                if (msg_id != "RSG") report_error_and_exit("start: GS didn't send proper message.");
                if (status == "OK")
                {
                    // init player stats
                    has_ongoing_game = true;
                    n_letters = stoi(parsed_msg[2]);
                    max_errors = stoi(parsed_msg[3]);
                    trials_count = 0;
                    errors_count = 0;
                    current_word = vector<char>(n_letters, EMPTY);

                    // present initial game layout
                    cout << "New game started (max " + to_string(max_errors) + " attempts). Guess " + to_string(n_letters) + " letters word: " + current_word_representation(current_word);
                }
                else if (status == "NOK")
                {
                    cout << "There's already a player with the same PLID on a ongoing game!" << endl;
                }
                else if (status == "ERR")
                {
                    report_error_and_exit("Error: play: player sent message with wrong syntax.");
                }

                break;
            }
            case 1: // "<play or pl> <letter>\n"
            {
                // error check
                if (parsed_line.size() != 2)
                {
                    cout << "play: You must follow the correct syntax: <play or pl> <letter>" << endl;
                    break;
                }
                else if (!has_ongoing_game)
                {
                    cout << "play: You need to start a game first." << endl;
                    break;
                }
                else if (!is_a_letter(parsed_line[1]))
                {
                    cout << "play: You must specify just a letter." << endl;
                    break;
                }

                trials_count++;

                // prepare message to send: "PLG <PLID> <letter> <trial>\n"
                string letter = parsed_line[1];
                string msg = "PLG " + PLID + " " + letter + " " + to_string(trials_count) + "\n";

                char uppercase_letter = to_upper_case(letter[0]);
                
                // send message to server and receive another: "RLG <status> <trial> [n pos*]"
                string recvd_msg = UDP(GS_IP, GS_port, msg);
                vector<string> parsed_msg = split_string(recvd_msg);

                // execute command
                string msg_id = parsed_msg[0];
                string status = parsed_msg[1];
                if(msg_id != "RLG")
                {
                    report_error_and_exit("play: server sent wrong message");
                    trials_count--;
                }
                else if (status == "ERR") // message sent by the player has wrong syntax
                {
                    report_error_and_exit("play: Message sent by the player has wrong syntax or there is no ongoing game for the player " + PLID + ".");
                    trials_count--;
                }
                int recvd_trial = stoi(parsed_msg[2]);
                if (status == "OK")  // letter guess was successful
                {
                    // get number of correct letters and their postitions
                    int n = stoi(parsed_msg[3]);

                    // fill current word with guessed letter
                    for (int i=0; i<n; i++)
                    {
                        int pos = stoi(parsed_msg[4+i]);
                        current_word[pos-1] = uppercase_letter; //? pos-1
                    }

                    // display result
                    cout << "Yes, ";
                    cout << uppercase_letter;
                    cout <<" is part of the word: ";
                    cout << current_word_representation(current_word);
                }
                else if (status == "WIN") // letter guess was successful and completes the word, also "[n pos*]" not present
                {
                    // fill current word with guessed letter on the empty postitions
                    for (int i=0; i<n_letters; i++)
                        if (current_word[i] == EMPTY)
                            current_word[i] = uppercase_letter;

                    // display result
                    cout << "WELL DONE! You guessed: ";
                    cout << current_word_representation(current_word);
                    has_ongoing_game = false;
                }
                else if (status == "DUP") // letter guess was already made
                {
                    cout << "you've already guessed this letter" << endl;
                    trials_count--;
                }
                else if (status == "NOK") // letter guess was not successful and max_errors hasn't been reached
                {
                    if (++errors_count > max_errors) report_error_and_exit("play: errors_count exceeded max_errors when it was not supposed to!");
                    cout << "play: Your letter guess is wrong, you have " + to_string(max_errors-errors_count) + " wrong attempts left" << endl;
                }
                else if (status == "OVR") // letter guess was not successful and max_errors has been reached
                {
                    cout << "play: Your letter guess is wrong, you have no wrong attempts left, GAME OVER." << endl;
                    has_ongoing_game = false;
                }
                else if (status == "INV") // trial number is not valid
                {
                    report_error_and_exit("play: trial number is not valid, player trial is " + to_string(trials_count) + ", but server trials is " + to_string(recvd_trial));
                    trials_count--;
                    // TODO: "or if the player is repeating the last PLG stored at the GS with a different letter"
                }
                
                break;
            }
            case 2: // "<guess or gw> <word>\n"
            {
                // error check
                if (parsed_line.size() != 2)
                {
                    cout << "guess: You must follow the correct syntax: <guess or gw> <word>" << endl;
                    break;
                }
                else if (!is_valid_guess(parsed_line[1], n_letters))
                {
                    cout << "guess: You must specify an actual word with " + to_string(n_letters) + "." << endl;
                    break;
                }
                else if (!has_ongoing_game)
                {
                    cout << "guess: You need to start a game first!" << endl;
                    break;
                }

                trials_count++;
                
                // prepare the message to send: "PWG <PLID> <word> <trial>\n"
                string word = parsed_line[1];
                string msg = "PWG " + PLID + " " + word + " " + to_string(trials_count) + "\n";
                cout << "guess: to send: " + msg << endl;


                // send message to server and receive another: "RWG <status> <trials>\n"
                string recvd_msg = UDP(GS_IP, GS_port, msg);
                vector<string> parsed_msg = split_string(recvd_msg);

                // execute command
                string msg_id = parsed_msg[0];
                string status = parsed_msg[1];
                if (status == "ERR")
                {
                    report_error_and_exit("Error: guess: message sent by the player has wrong syntax or there is no ongoing game for the player " + PLID + "\n");
                }
                int recvd_trial = stoi(parsed_msg[2]);
                if (msg_id != "RWG")
                {
                    report_error_and_exit("Error: guess: server sent wrong message");
                }
                else if (status == "WIN") // word guess was successful
                {
                    // fill current_word
                    for (int i=0; i<n_letters; i++)
                        current_word[i] = to_upper_case(word[i]);

                    // display word
                    cout << "WELL DONE! You guessed: ";
                    cout << current_word_representation(current_word);

                    // end game
                    has_ongoing_game = false;
                }
                else if (status == "DUP") // word guess was already made
                {
                    cout << "you've already guessed this word" << endl;
                    trials_count--;
                }
                else if (status == "NOK") // word guess was not successful and max_errors hasn't been reached
                {   
                    cout << "your word guess is wrong, you have " + to_string(max_errors-errors_count) + " wrong attempts left" << endl;
                    errors_count++;
                }
                else if (status == "OVR") // word guess was not successful and max_errors has been reached
                {
                    cout << "your word guess is wrong, you have no wrong attempts left, GAME OVER" << endl;
                    has_ongoing_game = false;
                }
                else if (status == "INV")
                {
                    report_error_and_exit("Error: guess: trial number is not valid, player trial is " + to_string(trials_count) + ", but server trials is " + to_string(recvd_trial));
                }
                
                break;
            }
            case 3: // "<scoreboard or sb>\n"
            {
                // error check

                // prepare message to send: "GSB\n"
                string msg = "GSB\n";

                // send message to server and receive another: "RSB <status> [Fname Fsize Fdata]\n"
                vector<string> parsed_msg;
                TCP(GS_IP, GS_port, msg, parsed_msg);
                
                // execute command
                string msg_id = parsed_msg[0];
                if (msg_id == "ERR")
                {
                    report_error_and_exit("state: player message has wrong syntax!");
                }
                string status = parsed_msg[1];
                if (msg_id != "RSB")
                {
                    report_error_and_exit("scoreboard: server sent wrong message!");
                }
                else if (status == "EMPTY") // scoreboard is empty, [Fname Fsize Fdata] don't exist
                {
                    cout << "scoreboard is empty" << endl;
                }
                else if (status == "OK") // scoreboard is not empty
                {
                    string Fname = parsed_msg[2];
                    ifstream file(Fname);
                    string line;
                    while (file.is_open() && getline(file, line))
                    {
                        cout << line << endl;
                    }
                    file.close();
                }

                break;
            }
            case 4: // "<hint or h>\n"
            {
                // error check

                // prepare message to send: "GHL <PLID>\n"
                string msg = "GHL " + PLID + "\n";

                // send message to server and receive another: "RHL <status> [Fname Fsize Fdata]\n"
                vector<string> parsed_msg;
                TCP(GS_IP, GS_port, msg, parsed_msg);

                // execute commnad
                string msg_id = parsed_msg[0];
                if (msg_id == "ERR")
                {
                    report_error_and_exit("state: player message has wrong syntax!");
                }
                string status = parsed_msg[1];
                if (msg_id != "RHL")
                {
                    report_error_and_exit("hint: server sent wrong message!");
                }
                else if (status == "OK") // image sent successfully
                {
                    string Fname = parsed_msg[2];
                    string Fsize = parsed_msg[3];
                    cout << "Fname: " + Fname + "; Fzise: " + Fsize + "\n";
                }
                else if (status == "NOK") // no image to send or some other problem
                {
                    cout << "there is no file to be sent! (or some other problem)" << endl;
                }

                break;
            }
            case 5: // "<state or st>\n"
            {
                // error check

                // prepare message to send: "STA <PLID>\n"
                string msg = "STA " + PLID + "\n";

                // send message to server and receive another: RST <status> [Fname Fsize Fdata]
                vector<string> parsed_msg;
                TCP(GS_IP, GS_port, msg, parsed_msg);

                // execute command
                string msg_id = parsed_msg[0];
                if (msg_id == "ERR")
                {
                    report_error_and_exit("state: player message has wrong syntax!");
                }
                string status = parsed_msg[1];
                if (msg_id != "RST")
                {
                    report_error_and_exit("state: server sent wrong message!");
                }
                else if (status == "ACT") // player has an ongoing game
                {
                    string Fname = parsed_msg[2];
                    ifstream file(Fname);
                    string line;
                    while (file.is_open() && getline(file, line))
                    {
                        cout << line << endl;
                    }
                    file.close();
                }
                else if (status == "FIN") // player has no ongoing game
                {
                    string Fname = parsed_msg[2];
                    ifstream file(Fname);
                    string line;
                    while (file.is_open() && getline(file, line))
                    {
                        cout << line << endl;
                    }
                    file.close();
                    

                    //TODO: terminate game
                }
                else if (status == "NOK") // server finds no games (active or finished) for the player
                {
                    cout << "server didn't find any games" << endl;
                }

                break;
            }
            case 6: // "<quit or exit>\n"
            {
                if (parsed_line[0] == "exit") exit = true;

                if (has_ongoing_game)
                {
                    // prepare message to send: "QUT <PLID>\n"
                    string msg = "QUT " + PLID + "\n";

                    // send message to server and receive another: "RQT <status>"
                    string recvd_msg = UDP(GS_IP, GS_port, msg);
                    vector<string> parsed_msg = split_string(recvd_msg);

                    // execute command
                    string msg_id = parsed_msg[0];
                    string status = parsed_msg[1];
                    if (msg_id != "RQT") report_error_and_exit("quit: server sent wrong message");
                    if (status == "OK") 
                    {
                        cout << "quit: Game successfuly terminated." << endl;
                        has_ongoing_game = false;
                    }
                    else if (status == "NOK")
                    {
                        cout << "quit: There isn't a player with that PLID on an ongoing game!" << endl;
                    }
                    else if (status == "ERR")
                    {
                        report_error_and_exit("Error: quit: Failed to quit.");
                    }
                }

                break;
            }
            case 7: // "rev\n"
            {
                // error check
                //if (!has_ongoing_game)
                //{
                //    cout << "you need to start a game first!" << endl;
                //    break;
                //}

                // prepare message to send: "REV <PLID>\n
                string msg = "REV " + PLID + "\n";

                // send messgae to server and receive another: "RRV <word for now>\n"
                string recvd_msg = UDP(GS_IP, GS_port, msg);
                vector<string> parsed_msg = split_string(recvd_msg);

                // execute command
                string msg_id = parsed_msg[0];
                string word_to_be_guessed = parsed_msg[1];
                if (parsed_msg[0] != "RRV") report_error_and_exit("rev: GS didn't send proper message");
                cout << "word to be guessed: " + word_to_be_guessed << endl;

                break;
            }
            default:
            {
                cout << "You must give a valid command." << endl;
                break;
            }

        }
        if (exit) break;
    }

    exit(0);
}