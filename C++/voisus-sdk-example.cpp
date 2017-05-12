#include "vrcc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
#endif

using namespace std;

int Current_radio;
char Last_cmd[1024];

void get_help(void);
void connect(void);
void disconnect(void);
void get_radio(void);
void get_radio_nets(void);
void get_radios(void);
void get_roles(void);
void get_status(void);
void set_client_name(void);
void set_ptt(void);
void set_radio(void);
void set_radio_net(void);
void set_role(void);
void set_rx_enable(void);
void set_tx_enable(void);
void quit_app(void);

typedef void (*samplefunc)();

typedef struct {
    const char* name;
    const char* desc;
    samplefunc func;
} COMMAND_T;

COMMAND_T Commands[] = {{"get_help", "Print command descriptions", get_help},
                        {"connect", "Connect to server", connect},
                        {"disconnect", "Disconnect from server", disconnect},
                        {"get_radio", "Get current radio info", get_radio},
                        {"get_radio_nets", "Get nets assigned to current radio", get_radio_nets},
                        {"get_radios", "Get info on all radios", get_radios},
                        {"get_roles", "Get list of roles", get_roles},
                        {"get_status", "Get current status", get_status},
                        {"set_client_name", "Set client name", set_client_name},
                        {"set_ptt", "Set PTT state (pressed or released)", set_ptt},
                        {"set_radio", "Set the current radio by index", set_radio},
                        {"set_radio_net", "Set the net for a radio by index", set_radio_net},
                        {"set_role", "Set the role to use", set_role},
                        {"set_rx_enable", "Set receive enable for current radio", set_rx_enable},
                        {"set_tx_enable", "Set transmit enable for current radio", set_tx_enable},
                        {"quit", "Quit the application", quit_app},
                        {NULL, NULL, NULL}};

void my_sleep(unsigned int ms)
{
#ifdef WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

size_t get_input(char* buf, size_t bufsz)
{
    int sz = 0;
    while (1)
    {   sz = read(STDIN_FILENO, buf, bufsz);
        if (-1 != sz)
        {   buf[sz-1] = '\0';
            return sz;
        }
        VRCC_Update(); // Must be called periodically to get updates
        my_sleep(50);
    }
    return 0;
}

void check_server(void)
{
    if (TARGET_CONNECT == Network_ConnectState())
        printf("WARNING: Not connected to server.\n");
}

void check_connected(void)
{
    if (ROLE_CONNECTED != Network_ConnectState())
        printf("WARNING: Not connected to role.\n");
}

void execute(const char* buf)
{
    COMMAND_T* cmd = Commands;
    if ((0 == strlen(buf)) && (strlen(Last_cmd)))
        buf = Last_cmd;
    while (cmd->name)
    {   if (0 == strcmp(cmd->name, buf))
        {   check_server();
            check_connected();
            cmd->func();
            snprintf(Last_cmd, sizeof(Last_cmd), cmd->name);
            return;
        }
        ++cmd;
    }
    printf("Unknown command. Type get_help to see available commands.");
}

void print_radio(int idx)
{
    printf("Radio index %d:%s\n"
           "    Active Net: %s\n"
           "    Transmit Enabled: %s\n"
           "    Receive Enabled: %s\n"
           "    Receiving: %s\n"
           "    Transmitting: %s\n",
           idx,
           (idx == Current_radio) ? " (*** Current ***)" : "",
           Radio_NetNameActive(idx),
           Radio_IsTransmitEnabled(idx) ? "true" : "false",
           Radio_IsReceiveEnabled(idx) ? "true" : "false",
           Radio_IsReceiving(idx) ? "true" : "false",
           Radio_IsTransmitting(idx) ? "true" : "false");
}

void get_help(void)
{
    COMMAND_T* cmd = Commands;
    while (cmd->name)
    {   printf("%-20s %s\n", cmd->name, cmd->desc);
        cmd++;
    }
}

void connect(void)
{
    char ip[32];
    printf("Enter IP address of server: ");
    fflush(stdout);
    get_input(ip, sizeof(ip));
    Voisus_ConnectServer(ip);
}

void disconnect(void)
{
    Voisus_Disconnect();
}

void get_radio(void)
{
    if (Current_radio < Radio_ListCount())
        print_radio(Current_radio);
}

void get_radio_nets(void)
{
    printf("Nets assigned to Radio %d:\n", Current_radio);
    for (int i = 0; i < Radio_NetListCount(Current_radio); i++)
    {   int current_net = !strcmp(Radio_NetID(Current_radio, i),
                                  Radio_NetIDActive(Current_radio));
        printf("Net index %d:%s\n"
               "    Name: %s\n"
               "    Frequency: %llu Hz\n"
               "    Waveform: %s\n"
               "    Crypto enabled: %s\n",
               i,
               current_net ? " (*** Current ***)" : "",
               Radio_NetName(Current_radio, i),
               Radio_NetFrequency(Current_radio, i),
               Radio_NetWaveform(Current_radio, i),
               Radio_NetCryptoEnabled(Current_radio, i) ? "true" : "false");
    }
}

void get_radios(void)
{
    for (int i = 0; i < Radio_ListCount(); i++)
        print_radio(i);
}

void get_roles(void)
{
    for (int i = 0; i < Role_ListCount(); i++)
        printf("    Role %d:\t%s\n", i, Role_Name(i));
}

void get_status(void)
{
    printf("Voisus Server IP Address: %s\n"
           "Client Name: %s\n"
           "Connection State: %d\n"
           "Connection Status: %s\n"
           "Role: %s\n",
           Network_TargetIP(),
           Network_ClientName(),
           Network_ConnectState(),
           Network_ConnectionStatus() == STATUS_CONNECTED ? "Connected" : "Disconnected",
           Role_NameActive());
}

void set_client_name(void)
{
    char name[32];
    printf("Enter client name: ");
    fflush(stdout);
    get_input(name, sizeof(name));
    Network_SetClientName(name);
}

void set_ptt(void)
{
    printf("Setting Push-To-Talk state to %s.\n",
           PTT_GetPressed() ? "Released" : "Pressed");
    PTT_SetPressed(!PTT_GetPressed());
}

void set_radio(void)
{
    char idxstr[32];
    printf("Enter radio number (see: get_radios): ");
    fflush(stdout);
    get_input(idxstr, sizeof(idxstr));
    int idx = atoi(idxstr);
    if (idx < Radio_ListCount())
    {   Current_radio = idx;
        get_radios();
    }
    else
        printf("Bad radio index. Current radio index is %d.\n", Current_radio);
}

void set_radio_net(void)
{
    char idxstr[32];
    if (0 == Radio_ListCount())
    {   printf("No radios.");
        return;
    }
    printf("Enter net number (see: get_radio_nets): ");
    fflush(stdout);
    get_input(idxstr, sizeof(idxstr));
    int idx = atoi(idxstr);
    if (idx < Radio_NetListCount(Current_radio))
        Radio_SetNet(Current_radio, idx);
    else
        printf("Bad net index.\n");
}

void set_role(void)
{
    char idxstr[32];
    printf("Enter role number (see: get_roles): ");
    fflush(stdout);
    get_input(idxstr, sizeof(idxstr));
    int idx = atoi(idxstr);
    const char* role_id = Role_Id(idx);
    if (strlen(role_id))
    {   Role_SetRole(role_id);
        printf("Set role to %s.\n", Role_Name(idx));
    }
    else
        printf("Unknown role.\n");
}

void set_rx_enable(void)
{
    if (Current_radio >= Radio_ListCount())
        return;
    printf("Setting Receive Enable of Radio %d to %s.\n",
           Current_radio,
           Radio_IsReceiveEnabled(Current_radio) ? "Disabled" : "Enabled");
    Radio_SetReceiveEnabled(Current_radio,
                            !Radio_IsReceiveEnabled(Current_radio));
}

void set_tx_enable(void)
{
    if (Current_radio >= Radio_ListCount())
        return;
    printf("Setting Transmit Enable of Radio %d to %s.\n",
           Current_radio,
           Radio_IsTransmitEnabled(Current_radio) ? "Disabled" : "Enabled");
    Radio_SetTransmitEnabled(Current_radio,
                             !Radio_IsTransmitEnabled(Current_radio));
}

void quit_app(void)
{
    exit(0);
}

int input_available()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(0, &fds);
}

int main(int argc, char* argv[])
{
    printf("VRCC C/C++ Library Sample Application\n\n");
    printf("Type get_help to see available commands.\n\n");

    VRCC_Start(argc, argv);

    char cmd[1024];
    printf("> ");
    fflush(stdout);
    while(1)
    {   if (input_available())
        {   get_input(cmd, sizeof(cmd));
            execute(cmd);
            printf("\n> ");
            fflush(stdout);
        }
        else
        {   VRCC_Update(); // Must be called periodically to get updates
            my_sleep(50);
        }
    }

    printf("Done.\n");

    return 0;
}