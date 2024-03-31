#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <ctime>
#include <filesystem>

#define max(A, B) ((A) >= (B)) ? (A) : (B)

#define NUM_WORDS 26
#define BUFFER_SIZE 128
#define FILE_MAX_SIZE 1024 * 1024 * 1024

using namespace std;

//**************************************
//********** Aux String Functions ******
//**************************************

char* turn_into_cstring(string str)
{
    char* c_str = new char [str.length()+1];
    strcpy(c_str, str.c_str());
    return c_str;
}

string turn_into_cppstring(char* str, int n)
{
    string cpp_string = str;
    return cpp_string.substr(0, n);
}

vector<string> split_string(string str)
{
    vector<string> splitted;
    
    char* c_str = turn_into_cstring(str);

    char* c_token = strtok(c_str, " ");
    while (c_token != NULL)  
    {  
        string token = c_token;
        splitted.push_back(token);
        c_token = strtok(NULL, " \n:");  
    }
    
    return splitted;
}

string trim_until_new_line(string str)
{
    // find '\n' index
    int i;
    for (i=0; i<128; i++)
        if (str[i] == '\n')
            break;

    return str.substr(0, i);
}

//**************************************
//********** Error Functions ***********
//**************************************

void report_error_and_exit(string err_msg)
{
    cerr << err_msg << endl;
    exit(1);
}

//**************************************
//********** Other Functions ***********
//**************************************

int get_option(string value)
{
         if (value == "SNG") {return 0;}
    else if (value == "PLG") {return 1;}
    else if (value == "PWG") {return 2;}
    else if (value == "GSB") {return 3;}
    else if (value == "GHL") {return 4;}
    else if (value == "STA") {return 5;}
    else if (value == "QUT") {return 6;}
    else if (value == "REV") {return 7;}   
    else                     {return 8;}
}

string get_current_time_and_date()
{
    time_t t;
    struct tm * ti;

    time (&t);
    ti = localtime (&t);
    return asctime(ti); // <weekday> <month> <day> <hour>:<minute>:<second> <year>
}

//**************************************
//********** Connections   *************
//**************************************

int file_size(string file_name)
{
    ifstream file;
    file.open(file_name, ios::binary);
    file.seekg(0, ios::end);
    int size = file.tellg(); // in bytes
    file.close();
    return size;
}

void UDP()
{
    int fd, errcode;
    ssize_t n, n_read, n_written;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    // open socket
    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1) report_error_and_exit("Error: UDP: Failed to open UDP socket.");

    // ...
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE;

    // get client's address info
    errcode = getaddrinfo(NULL, "58094", &hints, &res);
    if (errcode != 0) report_error_and_exit("Error: UDP: Failed to get client's address info.");

    // establish connection with client
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) report_error_and_exit("Error: UDP: Failed to establish connection with client. There's already an UDP server on GS_port.");

    while (true)
    {
        addrlen = sizeof(addr);
        
        // receive message from client
        n_read = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
        if (n_read == -1) report_error_and_exit("Error: UDP: Failed to receive message from client.");
        write(1, "received UDP: ", 10); write(1, buffer, n_read);

        // reply to client
        n = sendto(fd, buffer, n_read, 0, (struct sockaddr*) &addr, addrlen);
        if (n == -1) report_error_and_exit("Error: UDP: Failed to reply to client.");
    }

    // close stuff
    freeaddrinfo(res);
    close(fd);

    return;
}

void UDP_setup(string GS_port, int & fd, struct addrinfo & hints, struct addrinfo* & res)
{
    int errcode;
    ssize_t n;

    // open socket
    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1) report_error_and_exit("Error: UDP: Failed to open UDP socket.");

    // ...
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE;

    // establish server's port and get client's address info
    errcode = getaddrinfo(NULL, GS_port.c_str(), &hints, &res);
    if (errcode != 0) report_error_and_exit("Error: UDP: Failed to get client's address info.");

    // establish connection with client
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) report_error_and_exit("Error: UDP: Failed to bind. There's already an UDP or TCP server on GS_port.");


}

string UDP_receive(int & fd, struct sockaddr_in & addr, socklen_t & addrlen, struct addrinfo & hints, struct addrinfo* & res)
{
    int n_read;
    char buffer[128];

    // receive message from client
    string recvd_msg = "";
    addrlen = sizeof(addr);
    n_read = recvfrom(fd, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n_read == -1) report_error_and_exit("Error: UDP: Failed to receive message from client.");
    recvd_msg.append(buffer);
    
    // trim received message
    string trimmed_recvd_msg = trim_until_new_line(recvd_msg);
    cout << "received UDP: " + trimmed_recvd_msg << endl;
    
    return trimmed_recvd_msg;
}

void UDP_reply(string reply, int & fd, struct sockaddr_in & addr, socklen_t & addrlen, struct addrinfo* & res)
{
    int n_written;

    // reply to client
    n_written = sendto(fd, turn_into_cstring(reply), reply.length(), 0, (struct sockaddr*) &addr, addrlen);
    if (n_written == -1) report_error_and_exit("Error: UDP: Failed to reply to client.");
    
    // close stuff
    freeaddrinfo(res);
    close(fd);
}

void TCP()
{
    int fd, new_fd, errcode;
    ssize_t n, n_read, n_written;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128], *ptr;

    // open socket
    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1) report_error_and_exit("Error: TCP: Failed to open socket.");

    // ...
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE;

    // get client's address info
    errcode = getaddrinfo(NULL, "58094", &hints, &res);
    if (errcode != 0) report_error_and_exit("Error: TCP: Failed to get client's address info.");

    // register server's address info to the system
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) report_error_and_exit("Error: TCP: Failed to establish connection with client. There's already and UDP or TCP server on GS_port.");

    // listen for incoming connection requests for the opened socket
    if (listen(fd, 5) == -1) report_error_and_exit("Error: TCP: Failed to listen.");

    while (true)
    {
        addrlen = sizeof(addr);

        // extract the first connection request on the queue of pending connections
        new_fd = accept(fd, (struct sockaddr*) &addr, &addrlen);
        if (new_fd == -1) report_error_and_exit("...");

        // receive message from client
        while ((n_read = read(new_fd, buffer, 128)) != 0)
        {
            if (n_read == -1) report_error_and_exit("Error: TCP: Failed to receive part of the message from client.");
            ptr = &buffer[0];
            
            // reply to client
            while (n_read > 0)
            {
                n_written = write(new_fd, ptr, n_read);
                if (n_written <= 0) report_error_and_exit("Error: TCP: Failed to sent part of the reply to client.");
                n_read -= n_written;
                ptr += n_written;
            }

            break;
        }

        close(new_fd);
    }

    // close stuff
    freeaddrinfo(res);
    close(fd);

    return;
}

void TCP_setup(string GS_port, int & fd, struct addrinfo & hints, struct addrinfo* & res)
{
    int errcode, n;
    
    // open socket
    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1) report_error_and_exit("Error: TCP: Failed to open socket.");

    // ...
    memset(&hints, 0 , sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE;

    // get client's address info
    errcode = getaddrinfo(NULL, GS_port.c_str(), &hints, &res);
    if (errcode != 0) report_error_and_exit("Error: TCP: Failed to get client's address info.");

    // register server's address info to the system
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) report_error_and_exit("Error: TCP: Failed to bind.");
}

string TCP_receive(int & fd, int & new_fd, struct sockaddr_in & addr, socklen_t & addrlen, struct addrinfo & hints, struct addrinfo* & res)
{
    int n_read;
    char buffer[128];
    
    // listen for incoming connection requests for the opened socket
    if (listen(fd, 5) == -1) report_error_and_exit("Error: TCP: Failed to listen.");

    addrlen = sizeof(addr);
        
    // extract the first connection request on the queue of pending connections
    new_fd = accept(fd, (struct sockaddr*) &addr, &addrlen);
    if (new_fd == -1) report_error_and_exit("Error: TCP: failed to extract frist connection");

    // receive message from client
    string recvd_msg = "";
    while (true)
    {
        n_read = read(new_fd, buffer, 128);
        if (n_read == -1) report_error_and_exit("Error: TCP: failed to receive part of the message from client");
        else if (n_read == 0) break;
        recvd_msg.append(buffer);
    }

    // trim received message
    string trimmed_recvd_msg = trim_until_new_line(recvd_msg);
    cout << "received TCP: " + trimmed_recvd_msg << endl;
    
    return trimmed_recvd_msg;
}

void TCP_reply(string & reply, int & new_fd)
{                      
    int n_written;
    
    int bytes_to_write = reply.length();
    char* c_reply = turn_into_cstring(reply);
    char* ptr = &c_reply[0];

    while (bytes_to_write > 0)
    {
        n_written = write(new_fd, ptr, bytes_to_write);
        if (n_written == -1) report_error_and_exit("Error: TCP: Failed to send part of the reply to client.");
        else if (n_written == 0);
        bytes_to_write -= n_written;
        ptr += n_written;
    }
}

void TCP_reply_char_array(char* buffer, int bytes_to_write, int & new_fd)
{                      
    int n_written;
    
    buffer[bytes_to_write] = '\n';
    bytes_to_write++;

    char* ptr = &buffer[0];

    while (bytes_to_write > 0)
    {
        n_written = write(new_fd, ptr, bytes_to_write);
        if (n_written == -1) report_error_and_exit("Error: TCP: Failed to send part of the reply to client.");
        else if (n_written == 0);
        bytes_to_write -= n_written;
        ptr += n_written;
    }
}

void TCP_with_select(char* GS_port, string msg)
{
    int fd, new_fd, afd=0, max_fd, counter, errcode;
    fd_set rfds;
    ssize_t n, n_read, n_written;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128], *ptr;
    enum {idle, busy} state;

    // open socket
    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1) report_error_and_exit("Error: TCP: Failed to open socket.");

    // ...
    memset(&hints, 0 , sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE;

    // get client's address info
    errcode = getaddrinfo(NULL, GS_port, &hints, &res);
    if (errcode != 0) report_error_and_exit("Error: TCP: Failed to get client's address info.");
    
    // register server's address info to the system
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) report_error_and_exit("Error: TCP: Failed to establish connection with client.");

    // listen for incoming connection requests for the opened socket
    if (listen(fd, 5) == -1) report_error_and_exit("Error: TCP: Failed to listen.");

    state = idle;

    while(true)
    {
        // set file descriptors
        FD_SET(fd, &rfds);
        max_fd = fd;
        if (state == busy)
        {
            FD_SET(afd, &rfds);
            max_fd = max(fd, afd);
        }

        // blocks until one of the file descriptors in rfds is ready to be read
        // counter: number of files descriptors ready
        counter = select(max_fd+1, &rfds, (fd_set*) NULL, (fd_set*) NULL, (struct timeval*) NULL);
        if (counter <= 0) report_error_and_exit("Error: TCP: select failed");
        
        if (FD_ISSET(fd, &rfds)) // fd is ready
        {
            addrlen = sizeof(addr);
            
            new_fd = accept(fd, (struct sockaddr*) &addr, &addrlen);
            if (new_fd == -1) report_error_and_exit("Error: TCP: Failed to open new_fd.");

            switch (state)
            {
                case idle:
                {
                    afd = new_fd;
                    state = busy;
                    break;
                }
                case busy:
                {
                    //TODO: write "busy\n" in newfd
                    close(new_fd);
                    break;
                }
            }
        }
        if (FD_ISSET(afd, &rfds)) // afd is ready
        {
            n_read = read(afd, buffer, 128);
            if (n_read != 0)
            {
                if (n_read == -1) report_error_and_exit("Error: TCP: ...");
                //TODO: write buffer in afd
            }
            else // connection closed by peer
            {
                close(afd);
                state = idle;
            }
        }

        // receive message from client
        while ((n_read = read(new_fd, buffer, 128)) != 0)
        {
            if (n_read == -1) report_error_and_exit("Error: TCP: Failed to receive part of the message from client.");
            ptr = &buffer[0];
            
            // reply to client
            while (n_read > 0)
            {
                n_written = write(new_fd, ptr, n_read);
                if (n_written <= 0) report_error_and_exit("Error: TCP: Failed to sent part of the reply to client.");
                n_read -= n_written;
                ptr += n_written;
            }

        }

        close(new_fd);        
    }

    // close stuff
    freeaddrinfo(res);
    close(fd);

    return;
}

void TCP_with_fork(char* GS_port, string msg)
{
    int fd, new_fd, errcode;
    ssize_t n, n_read, n_written;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128], *ptr;
    struct sigaction act;
    pid_t pid;

    // ???
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &act, NULL) == -1) report_error_and_exit("zombie child process");

    // open socket
    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1) report_error_and_exit("Error: TCP: Failed to open socket.");

    // ...
    memset(&hints, 0 , sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE;

    // get client's address info
    errcode = getaddrinfo(NULL, GS_port, &hints, &res);
    if (errcode != 0) report_error_and_exit("Error: TCP: Failed to get client's address info.");

    // register server's address info to the system
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) report_error_and_exit("Error: TCP: Failed to establish connection with client.");

    // listen for incoming connection requests for the opened socket
    if (listen(fd, 5) == -1) report_error_and_exit("Error: TCP: Failed to listen.");

    freeaddrinfo(res);

    while (true)
    {
        addrlen = sizeof(addr);

        // extract the first connection request on the queue of pending connections
        do {
            new_fd = accept(fd, (struct sockaddr*) &addr, &addrlen);
        } while (new_fd == -1 && errno == EINTR);
        if (new_fd == -1) report_error_and_exit("...");

        // create a child process for each new connection
        pid = fork();
        if (pid == -1) report_error_and_exit("Error: TCP: failed to fork.");
        else if (pid == 0) // child process execution
        {
            close(fd);
            
            // receive message from client
            while ((n_read = read(new_fd, buffer, 128)) != 0)
            {
                if (n_read == -1) report_error_and_exit("Error: TCP: Failed to receive part of the message from client.");
                ptr = &buffer[0];

                // reply to client
                while (n_read > 0)
                {
                    n_written = write(new_fd, ptr, n_read);
                    if (n_written <= 0) report_error_and_exit("Error: TCP: Failed to sent part of the reply to client.");
                    n_read -= n_written;
                    ptr += n_written;
                }

            }

            close (new_fd);
            exit(0);
        }

        // parent process execution
        do {
            errcode = close(new_fd);
        } while (errcode == -1 && errno == EINTR);
        if (errcode == -1) report_error_and_exit("...");
    }

    close(fd);

    return;
}

//**************************************
//******** Words Functions  ************
//**************************************

vector<vector<string>> create_words_table(string file_name)
{
    // allocate
    vector<vector<string>> words(NUM_WORDS, vector<string>(2, ""));

    // get words file
    ifstream file;
    file.open(file_name);

    // fill
    string line;
    for (int i=0; i<NUM_WORDS && getline(file, line); i++)
    {
        vector<string> parsed_line = split_string(line);
        words[i][0] = parsed_line[0];
        words[i][1] = parsed_line[1];
    }

    return words;
}

vector<string> get_words_table_entry(vector<vector<string>> & words_table, int & entry_giver)
{
    if (entry_giver == NUM_WORDS) entry_giver = 0;
    
    vector<string> entry = words_table[entry_giver];
    
    entry_giver++;
    if (entry_giver == NUM_WORDS) entry_giver = 0;
    
    return entry;
}

int max_errors_giver(int n_letters)
{
    if      (    n_letters <=  6) {return 7;}
    else if (6 < n_letters <= 10) {return 8;}
    else                          {return 9;}
}

//**************************************
//**** Files & Directories Functions ***
//**************************************

bool file_or_dir_exists(string str)
{
    char* c_str = turn_into_cstring(str);
    struct stat sb;
    
    if (stat(c_str, &sb) == 0)
        return true;
    else
        return false;
}

void append_to_file(string file_name, string content)
{
    ofstream output_file;
    output_file.open(file_name, ios::app);
    output_file << content;
    output_file.close();
}

void end_game(string result, string PLID, string file_name)
{
    // create player directory if it doesn't exist
    string dir_name = PLID;
    if (!file_or_dir_exists(dir_name))
    {
        const char* dir = turn_into_cstring(PLID);
        mkdir(dir, 0777);
    }

    // get game's end date and time: "<weekday> <month> <day> <hour>:<minute>:<second> <year>"
    string time = get_current_time_and_date();
    vector<string> parsed_time = split_string(time);
    string year   = parsed_time[6];
    string month  = parsed_time[1];
    string day    = parsed_time[2];
    string hour   = parsed_time[3];
    string minute = parsed_time[4];
    string second = parsed_time[5];

    // rename and move player's file: "YYYYMMDD_HHMMSS_code.txt"
    string new_path = dir_name + "/" + year + month + day + "_" + hour + minute + second + "_" + result + ".txt";
    rename(file_name.c_str(), new_path.c_str());

    /*
    // register in scoreboard if it's a win
    if (result == "W")
    {
        // create file and path names "score_PLID_DDMMYYYY_HHMMSS.txt"
        string score;
        string score_file_name = "score_" + PLID + "_" + day + month + year + "_" + hour + minute + second + ".txt";
        string score_file_path = "SCORES/" + score_file_name;

        // create file
        ofstream score_file(score_file_path);

        // fill file

    }
    */
}

//**************************************
//********** Main          *************
//**************************************

string get_last_path_in_dir(string dir)
{
    struct stat sb;
    string last;
    for (const auto & entry : filesystem::directory_iterator(dir))
        last = entry.path();
    return last;
}

int main(int argc, char** argv)
{     
    // command line arguments reading: "./server <word_file> [-p GS_port] [-v]\n"
    string word_file = argv[1];
    string GS_port; 
    bool verbose_mode_enabled = false;
    if (argc == 2)  // default GS_port
    {
        GS_port = "58094";
    }
    else if (argc == 4) // specified [-p GS_port]
    {
        string p_flag = argv[2];
        if (p_flag != "-p") report_error_and_exit("init: you must follow the correct syntax: ./server <word_file> [-p GS_port] [-v]");

        GS_port = argv[3]; // TODO: error check
    }
    else if (argc == 5) // specified [-p GS_port] and [-v]
    {
        string p_flag = argv[2];
        string v_flag = argv[4];
        if (p_flag != "-p" || v_flag != "-v") report_error_and_exit("init: you must follow the correct syntax: ./server <word_file> [-p GS_port] [-v]");

        GS_port = argv[3]; // TODO: error check

        verbose_mode_enabled = true;
    }

    // words table
    vector<vector<string>> words_table = create_words_table(argv[1]);
    int entry_giver = 0;

    // make a process for each type of connection
    pid_t pid = fork();
    if (pid == -1) report_error_and_exit("main: failed to fork");
    else if (pid == 0) // UDP process
    {    
        cout << "TCP pid: " << pid << endl;

        // UDP vars
        int fd;
        ssize_t n, n_read, n_written;
        struct sockaddr_in addr; socklen_t addrlen;
        struct addrinfo hints, *res;

        while (true)
        {   
            // setup UDP
            UDP_setup(GS_port, fd, hints, res);
            
            // receive client's message
            string recvd_msg = UDP_receive(fd, addr, addrlen, hints, res);
            vector<string> parsed_msg = split_string(recvd_msg);
            
            switch (get_option(parsed_msg[0]))
            {
                case 0: // <start or st>; receive: "SNG <PLID>\n"; reply: "RSG <status> [n_letters max_errors]"
                {
                    string PLID = parsed_msg[1];

                    // error check
                    if (parsed_msg.size() != 2 || PLID.length() != 6) // status ERR
                    {
                        string reply = "RSG ERR\n";
                        UDP_reply(reply, fd, addr, addrlen, res);
                        break; 
                    }

                    // access respective file
                    string file_name = "GAME_" + PLID + ".txt";
                    ifstream file;
                    file.open(file_name);
                    bool has_ongoing_game = file.is_open();

                    // execute command
                    if (!has_ongoing_game) // status OK
                    {
                        vector<string> entry = get_words_table_entry(words_table, entry_giver);
                        string word = entry[0];
                        string hint = entry[1];

                        // create new file
                        ofstream file(file_name);
                        file << word + " " + hint + "\n";
                        file.close();

                        // prepare reply
                        int n_letters = word.length();
                        int max_errors = max_errors_giver(n_letters);
                        string reply = "RSG OK " + to_string(n_letters) + " " + to_string(max_errors) + "\n"; 

                        // send reply              
                        UDP_reply(reply, fd, addr, addrlen, res);
                    }
                    else // status NOK
                    {
                        // prepare reply
                        string reply = "RSG NOK\n";

                        // send reply 
                        UDP_reply(reply, fd, addr, addrlen, res);

                        break;
                    }


                    break;
                }
                case 1: // <play or pl>; receive: "PLG <PLID> <letter> <trial>\n"; reply: "RLG <status> <trial> [n pos*]\n"
                {
                    // get tokens from parsed message
                    string PLID = parsed_msg[1];
                    string letter = parsed_msg[2];
                    int recvd_trial = stoi(parsed_msg[3]);

                    // access respective file
                    string file_name = "GAME_" + PLID + ".txt";
                    ifstream file;
                    file.open(file_name);
                    bool has_ongoing_game = file.is_open();

                    // error check
                    if (parsed_msg.size() != 4 ||  PLID.length() != 6) // status ERR
                    {
                        string reply = "RLG ERR\n";
                        UDP_reply(reply, fd, addr, addrlen, res);
                        break;
                    }


                    // execute command
                    if (has_ongoing_game)
                    {  
                        // get word
                        string line;
                        getline(file, line);
                        string word = split_string(line)[0];

                        // trials counters
                        int total_trials               = 0;
                        int total_correct_trials       = 0;
                        int total_wrong_trials         = 0;
                        int total_play_trials          = 0;
                        int total_guess_trials         = 0;
                        int correct_play_trials        = 0;
                        int correct_guess_trials       = 0;
                        int wrong_play_trials          = 0;
                        int wrong_guess_trials         = 0;

                        // get correct play trials to win
                        string letters_in_word_no_duplicates = "";
                        for (char c : word)
                            if (word.find_first_of(c) != word.npos) // if c in word
                                if (letters_in_word_no_duplicates.find_first_of(c) == letters_in_word_no_duplicates.npos) // if c not already in letters
                                    letters_in_word_no_duplicates.push_back(c);
                        int correct_play_trials_to_win = letters_in_word_no_duplicates.length();

                        // iterate file lines
                        bool play_is_duplicate = false;
                        for (; getline(file, line); total_trials++)
                        {
                            vector<string> parsed_line = split_string(line);
                            string code = parsed_line[0];
                            string play = parsed_line[1];
                            if (code == "T") // play command trial
                            {
                                total_play_trials++;

                                // check if play is duplicate (if so, than guess also is)
                                if (letter == play) 
                                {
                                    play_is_duplicate = true;
                                }

                                // check if play is correct
                                if (word.find_first_of(play) != word.npos)
                                {
                                    correct_play_trials++;
                                    total_correct_trials++;
                                }
                                else
                                {
                                    wrong_play_trials++;
                                    total_wrong_trials++;
                                }
                            }
                            else if (code == "G") // guess command trial
                            {
                                total_guess_trials++;

                                // check if guess is correct
                                if (word == play) // this will never happen in this command
                                {
                                    correct_guess_trials++;
                                    total_correct_trials++;
                                }   
                                else
                                {
                                    wrong_guess_trials++;
                                    total_wrong_trials++;
                                }
                            }
                            else 
                            {
                                report_error_and_exit("play: wrong file format");
                            }
                        }

                        file.close();

                        // account for new play
                        bool play_is_successful = false;
                        total_trials++;
                        total_play_trials++;
                        int _n = 0;
                        string pos = "";
                        if (word.find_first_of(letter) != word.npos) // play is successful
                        {
                            play_is_successful = true;
                            correct_play_trials++;
                            total_correct_trials++;

                            // populate n and pos
                            for (int i=0; i<word.length(); i++)
                            {
                                char c = word[i];
                                char letter_char = letter[0];
                                if (c == letter_char)
                                {
                                    _n++;
                                    pos.append(to_string(i+1) + " "); // note that the positions start at 1 and not at 0
                                }
                            }
                            pos.pop_back(); // delete last space
                        }
                        else
                        {
                            wrong_play_trials++;
                            total_wrong_trials++;
                        }

                        // get max errors and remaining attempts
                        int max_errors = max_errors_giver(word.length());
                        int remaining_attempts = max_errors - total_wrong_trials;

                        string new_play = "T " + letter + "\n";                        
                        
                        // reply to client
                        if (total_trials != recvd_trial) // status INV
                        {
                            string reply = "RLG INV " + to_string(total_trials) + "\n";
                            UDP_reply(reply, fd, addr, addrlen, res);
                        }
                        else if (play_is_duplicate) // status DUP
                        {
                            string reply = "RLG DUP " + to_string(total_trials) + "\n";
                            UDP_reply(reply, fd, addr, addrlen, res);
                        }
                        else if (correct_play_trials == correct_play_trials_to_win) // status WIN
                        {
                            string reply = "RLG WIN " + to_string(total_trials) + "\n";
                            UDP_reply(reply, fd, addr, addrlen, res);
                            append_to_file(file_name, new_play);
                            end_game("W", PLID, file_name);
                        }
                        else if (play_is_successful) // status OK
                        {
                            string reply = "RLG OK " + to_string(total_trials) + " " + to_string(_n) + " " + pos + "\n";
                            UDP_reply(reply, fd, addr, addrlen, res);
                            append_to_file(file_name, new_play);
                        }
                        else if (!play_is_successful && remaining_attempts > 0) // status NOK
                        {
                            string reply = "RLG NOK " + to_string(total_trials) + "\n";
                            UDP_reply(reply, fd, addr, addrlen, res);
                            append_to_file(file_name, new_play);
                        }
                        else if (!play_is_successful && remaining_attempts == 0) // status OVR
                        {
                            string reply = "RLG OVR " + to_string(total_trials) + "\n";;
                            UDP_reply(reply, fd, addr, addrlen, res);
                            append_to_file(file_name, new_play);
                            end_game("F", PLID, file_name);
                        }
                        
                    }
                    else // status ERR
                    {
                        string reply = "RLG ERR\n";
                        UDP_reply(reply, fd, addr, addrlen, res);
                        file.close();
                    }

                    break;
                }
                case 2: // <guess or gw>; receive: "PWG <PLID> <word> <trial>\n"; reply: "RWG <status> <trial>\n"
                {
                    // get tokens from parsed message
                    string PLID = parsed_msg[1];
                    string guessed_word = parsed_msg[2];
                    int recvd_trial = stoi(parsed_msg[3]);

                    // error check
                    if (parsed_msg.size() != 4 ||  PLID.length() != 6) // status ERR
                    {
                        string reply = "RWG ERR\n";
                        UDP_reply(reply, fd, addr, addrlen, res);
                        break;
                    }

                    // access respective file
                    string file_name = "GAME_" + PLID + ".txt";
                    ifstream file;
                    file.open(file_name);
                    bool has_ongoing_game = file.is_open();

                    // execute command
                    if (has_ongoing_game)
                    {
                        // get word
                        string line;
                        getline(file, line);
                        string word = split_string(line)[0];

                        // trials counters
                        int total_trials = 0;
                        int total_wrong_trials = 0;

                        // iterate file lines
                        bool guess_is_duplicate = false;
                        for (; getline(file, line); total_trials++)
                        {
                            vector<string> parsed_line = split_string(line);
                            string code = parsed_line[0];
                            string play = parsed_line[1];
                            if (code == "T") // play command trial
                            {
                                // check if play is wrong
                                if (word.find_first_of(play) == word.npos)
                                {
                                    total_wrong_trials++;
                                }
                            }
                            else if (code == "G") // guess command trial
                            {
                                // check if guess is duplicate
                                if (guessed_word == play)
                                {
                                    guess_is_duplicate = true;
                                }
                                
                                // check if guess is wrong
                                if (word != play)
                                {
                                    total_wrong_trials++;
                                }
                            }
                            else 
                            {
                                report_error_and_exit("play: wrong file format");
                            }
                        }

                        file.close();

                        // account for new guess
                        bool guess_is_successful = false;
                        total_trials++;
                        if (guessed_word == word)
                            guess_is_successful = true;

                        // get max_errors and remaining errors
                        int max_errors = max_errors_giver(word.length());
                        int remaining_attempts = max_errors - total_wrong_trials;

                        string new_guess = "G " + guessed_word + "\n";

                        // reply to client
                        if (total_trials != recvd_trial) // status INV
                        {
                            string reply = "RWG INV " + to_string(total_trials) + "\n";
                            UDP_reply(reply, fd, addr, addrlen, res);
                        }
                        else if (guess_is_duplicate) // status DUP
                        {
                            string reply = "RWG DUP " + to_string(total_trials) + "\n";
                            UDP_reply(reply, fd, addr, addrlen, res);
                        }
                        else if (guess_is_successful) // status WIN
                        {
                            string reply = "RWG WIN " + to_string(total_trials) + "\n";
                            UDP_reply(reply, fd, addr, addrlen, res);
                            append_to_file(file_name, new_guess);
                            end_game("W", PLID, file_name);
                        }
                        else if (!guess_is_successful && remaining_attempts > 0) // status NOK
                        {
                            string reply = "RWG NOK " + to_string(total_trials) + "\n";
                            UDP_reply(reply, fd, addr, addrlen, res);
                            append_to_file(file_name, new_guess);
                        }
                        else if (!guess_is_successful && remaining_attempts == 0) // status OVR
                        {
                            string reply = "RLG OVR " + to_string(total_trials) + "\n";;
                            UDP_reply(reply, fd, addr, addrlen, res);
                            append_to_file(file_name, new_guess);
                            end_game("F", PLID, file_name);
                        }
                    }
                    else // status ERR
                    {
                        string reply = "RWG ERR\n";
                        UDP_reply(reply, fd, addr, addrlen, res);
                        file.close();
                    }
                    
                    break;
                }
                case 6: // <quit or exit>; receive: "QUT <PLID>\n"; reply: "RQT <status\n"
                {
                    string PLID = parsed_msg[1];


                    // access respective file
                    string file_name = "GAME_" + PLID + ".txt";
                    ifstream file;
                    file.open(file_name);
                    bool has_ongoing_game = file.is_open();

                    // execute command
                    if (has_ongoing_game) // status OK
                    {
                        end_game("Q", PLID, file_name);

                        // send reply
                        string reply = "RQT OK\n";
                        UDP_reply(reply, fd, addr, addrlen, res);
                    }
                    else if (!has_ongoing_game) // status NOK
                    {
                        string reply = "RQT NOK\n";
                        UDP_reply(reply, fd, addr, addrlen, res);
                    }
                    else{ // error check
                        string reply = "RQT ERR\n";
                        UDP_reply(reply, fd, addr, addrlen, res);
                        break;
                    }

                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
    else if (pid  > 0) // TCP  process
    {
        ofstream to_kill;
        to_kill.open("to_kill.txt", ios::app);
        to_kill << pid;
        to_kill << endl;
        to_kill.close();

        // TCP vars
        int fd, new_fd;
        ssize_t n, n_read, n_written;
        struct sockaddr_in addr; socklen_t addrlen;
        struct addrinfo hints, *res;
        char buffer[BUFFER_SIZE];

        TCP_setup(GS_port, fd, hints, res);

        // listen for incoming connection requests for the opened socket
        if (listen(fd, 5) == -1) report_error_and_exit("Error: TCP: Failed to listen.");

        while (true)
        {
            addrlen = sizeof(addr);

            // extract the first connection request on the queue of pending connections
            new_fd = accept(fd, (struct sockaddr*) &addr, &addrlen);
            if (new_fd == -1) report_error_and_exit("Error: TCP: failed to accept.");

            while ((n_read = read(new_fd, buffer, BUFFER_SIZE)) != 0)
            {   
                if (n_read == -1) report_error_and_exit("Error: TCP: Failed to receive part of the message from client.");
                
                // trim received message
                string recvd_msg = buffer;
                string trimmed_recvd_msg = trim_until_new_line(recvd_msg);
                cout << "received TCP: " + trimmed_recvd_msg << endl;

                vector<string> parsed_msg = split_string(recvd_msg);
                switch (get_option(parsed_msg[0]))
                {
                    /*
                    case 3: // <scoreboard or sb>; receive: "GSB\n"; send: "RSB <status> [Fname Fsize Fdata]\n" 
                    {
                        // get tokens from client's message
                        string msg_id = parsed_msg[0];

                        // error check
                        if (false) {} // reply "ERR\n"



                        break;
                    }
                    */
                    case 4: // "<hint or h>; receive: "GHL <PLID>\n"; reply: "RHL <status> [Fname Fsize Fdata]\n"
                    {
                        // get tokens from client's message
                        string msg_id = parsed_msg[0];
                        string PLID   = parsed_msg[1];



                        // access respective file
                        string file_name = "GAME_" + PLID + ".txt";
                        ifstream file;
                        file.open(file_name);
                        bool has_ongoing_game = file.is_open();

                        if (has_ongoing_game) // status OK
                        {
                            // get img name
                            string line;
                            getline(file, line);
                            string img_name = split_string(line)[1];
                            string img_path = "imgs/" + img_name;

                            // get img file
                            ifstream img_file;
                            img_file.open(img_path, ios::in | ios::binary);

                            char* buffer = (char*) malloc(FILE_MAX_SIZE * sizeof(char));

                            // memset(buffer, 0, FILE_MAX_SIZE);
                            img_file.read(buffer, FILE_MAX_SIZE);

                            // prepare reply
                            string Fname = img_name;
                            string Fsize = to_string(img_file.gcount());
                            img_file.close();
                            string reply = "RHL OK " + Fname + " " + Fsize + " ";

                            // send reply
                            TCP_reply(reply, new_fd);
                            TCP_reply_char_array(buffer, img_file.gcount(), new_fd);

                            free(buffer);
                        }
                        else // status NOK
                        {
                            string reply = "RHL NOK\n";
                            TCP_reply(reply, new_fd);
                        }

                        file.close();

                        break;
                    }
                    case 5: // "<state or st>; receive: "STA <PLID>\n"; reply: "RST <status> [Fname Fsize Fdata]\n"
                    {
                        // get tokens from client's message
                        string msg_id = parsed_msg[0];
                        string PLID   = parsed_msg[1];

                        // error check
                        if (false) {} // reply "ERR\n"

                        // access respective file
                        string file_name = "GAME_" + PLID + ".txt";
                        ifstream file;
                        file.open(file_name);
                        bool has_ongoing_game = file.is_open();

                        if (has_ongoing_game) // status ACT
                        {
                            string Fname = file_name;
                            string Fsize = to_string(file_size(Fname));
                            string Fdata((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
                            string reply = "RST ACT " + Fname + " " + Fsize + " " + Fdata + "\n";
                            TCP_reply(reply, new_fd);
                        }
                        else
                        {
                            file.close();

                            if (file_or_dir_exists(PLID)) // status FIN
                            {
                                // open most recent player's ended game file
                                string file_path = get_last_path_in_dir(PLID);
                                ifstream recent_file;
                                recent_file.open(file_path);

                                // prepare reply
                                string Fname = file_path.substr(file_path.find_first_of('/')+1);
                                string Fdata((istreambuf_iterator<char>(recent_file)), istreambuf_iterator<char>());
                                string Fsize = to_string(Fdata.length());
                                recent_file.close();
                                string reply = "RST FIN " + Fname + " " + Fsize + " " + Fdata + "\n";

                                TCP_reply(reply, new_fd);
                            }
                            else // status NOK
                            {
                                string reply = "RST NOK\n";
                                TCP_reply(reply, new_fd);
                            }
                        }

                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            
                break;
            }
            
            close(new_fd);
        }
        
        freeaddrinfo(res);
        close(fd);
    }

    exit(0);
}