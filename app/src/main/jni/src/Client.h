SocketClient client;

#define maxplayerCount 200

int startClient();
bool isConnected();
void stopClient();
bool initServer();
bool stopServer();

enum Mode {
    InitMode = 1,
    HackMode = 2,
    StopMode = 98,
    EspMode = 99,
};

struct Request {
    int Mode;
    bool m_IsOn;
    int value;
    int ScreenWidth;
    int ScreenHeight;
};

struct PlayerData {
    Vector3 Head;
    Vector3 Toe;
    bool IsCaido;
    float Distancia;
    int Health;
    bool Name;
    char PlayerName[64];
};

struct Response {
    bool Success;
    int PlayerCount;
    PlayerData Players[maxplayerCount];
};

int startClient(){
    for (int retry = 0; retry < 10; retry++) {
        client = SocketClient();
        if (client.Create()) {
            if (client.Connect()) {
                if (initServer()) {
                    return 0;
                }
            }
            client.Close();
        }
        usleep(200000); // 200ms
    }
    return -1;
}

bool isConnected(){
    return client.connected;
}

void stopClient() {
    if(client.created && isConnected()){
        stopServer();
        client.Close();
    }
}

bool initServer() {
    Request request{Mode::InitMode, true, 0};
    int code = client.send((void*) &request, sizeof(request));
    if(code > 0) {
        Response response{};
        size_t length = client.receive((void*) &response);
        if(length > 0) {
            return response.Success;
        }
    }
    return false;
}

bool stopServer() {
    Request request{Mode::StopMode};
    int code = client.send((void*) &request, sizeof(request));
    if(code > 0) {
        Response response{};
        size_t length = client.receive((void*) &response);
        if(length > 0) {
            return response.Success;
        }
    }
    return false;
}

void SendBool(int32_t number, bool ftr) {
    Request request{number, ftr};
    int code = client.send((void*) &request, sizeof(request));
    if (code > 0) {
        Response response{};
        size_t length = client.receive((void*) &response);
        if (length > 0) {
            ;
        }
    }
}

void SendFloat(int32_t number, float very) {
    Request request{number, true, static_cast<int>(very)};
    float code = client.send((void*) &request, sizeof(request));
    if (code > 0) {
        Response response{};
        size_t length = client.receive((void*) &response);
        if (length > 0) {
            return;
        }
    }
}

// ADDED: SendInt function for aimbot type
void SendInt(int32_t number, int value) {
    Request request{number, true, value};
    int code = client.send((void*) &request, sizeof(request));
    if (code > 0) {
        Response response{};
        size_t length = client.receive((void*) &response);
        if (length > 0) {
            return;
        }
    }
}

Response getData(int screenWidth, int screenHeight){
    Request request{Mode::EspMode, true, 0, screenWidth, screenHeight};
    int code = client.send((void*) &request, sizeof(request));
    if(code > 0){
        Response response{};
        size_t length = client.receive((void*) &response);
        if(length > 0){
            return response;
        }
    }
    Response response{false, 0};
    return response;
}