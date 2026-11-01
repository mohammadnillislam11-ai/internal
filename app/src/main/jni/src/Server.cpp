#include <dirent.h>
#include "Server.h"
#include "Includes/Encrypt/oxorany_include.h"
#include <android/log.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <atomic>
#include <thread>
#include <stdarg.h>
#include <fstream>
#include <cstring>
#include <sys/stat.h>
#include <mutex>
#include <chrono>
#include <signal.h>
#include <vector>

// Global log file and mutex
std::mutex log_mutex;
FILE* g_logFile = nullptr;
bool g_debugEnabled = false;

void WriteLog(const char* format, ...) {
    if (!g_debugEnabled || !g_logFile) return;
    
    std::lock_guard<std::mutex> lock(log_mutex);
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    fprintf(g_logFile, "[%s] ", timestamp);
    
    va_list args;
    va_start(args, format);
    vfprintf(g_logFile, format, args);
    va_end(args);
    
    fprintf(g_logFile, "\n");
    fflush(g_logFile);
}

uintptr_t last_failed_ptr = 0;

bool is_elf_header(uintptr_t addr) {
    if (addr == 0 || mem_fd <= 0) return false;
    unsigned char magic[4];
    magic[0] = Read<uint8_t>(addr);
    magic[1] = Read<uint8_t>(addr + 1);
    magic[2] = Read<uint8_t>(addr + 2);
    magic[3] = Read<uint8_t>(addr + 3);
    return (magic[0] == 0x7F && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F');
}

static uintptr_t s_working_facade_offset = 0;

uintptr_t FindCorrectLibBase() {
    if (target_pid <= 0 || mem_fd <= 0) return 0;

    char mapsPath[64];
    snprintf(mapsPath, sizeof(mapsPath), "/proc/%d/maps", target_pid);
    
    FILE* maps = fopen(mapsPath, "r");
    if (!maps) return 0;
    
    char line[512];
    std::vector<uintptr_t> elfBases;
    
    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "libil2cpp.so")) {
            uintptr_t start = 0;
            sscanf(line, "%lx", &start);
            if (is_elf_header(start)) {
                elfBases.push_back(start);
            }
        }
    }
    fclose(maps);
    
    if (elfBases.empty()) return 0;

    static const uintptr_t candidates[] = {
        0xB4C41A0, // Free Fire MAX
        0xA72E4E0, // Free Fire Standard
        0xA986B7C, // Free Fire MAX alternate
        0xA7384E0,
        0xA8E44E0
    };

    for (uintptr_t base : elfBases) {
        for (uintptr_t cand : candidates) {
            uintptr_t gameFacade = Read<uintptr_t>(base + cand);
            if (gameFacade > 0x100000000) {
                s_working_facade_offset = cand;
                return base;
            }
        }
    }
    
    return elfBases.size() > 1 ? elfBases[1] : elfBases[0];
}

uintptr_t currentBestTargetObj = 0;
std::atomic<bool> keepAiming{false};
std::thread aimThread;
Vector3 currentBestTargetPos = Vector3::Zero();
bool silen4a = true;

// ORIGINAL SILENT AIM LOGIC - NOT CHANGED
void ContinuousAim(uintptr_t oneself) {
    while (keepAiming) {
        uint64_t aimingInfo = Read<uint64_t>(oneself + 0xd80);
        Vector3 startPos = Read<Vector3>(aimingInfo + 0x4c);
        Vector3 dir = currentBestTargetPos - startPos;
        Write<Vector3>(aimingInfo + 0x40, dir);
        
        // OFFSET LOG FOR SILENT AIM
        static int logCount = 0;
        if (logCount++ % 30 == 0) {
            WriteLog("[SILENT AIM] aimingInfo=0x%lx startPos=(%.1f,%.1f,%.1f) target=(%.1f,%.1f,%.1f) dir=(%.1f,%.1f,%.1f)", 
                    aimingInfo, startPos.X, startPos.Y, startPos.Z,
                    currentBestTargetPos.X, currentBestTargetPos.Y, currentBestTargetPos.Z,
                    dir.X, dir.Y, dir.Z);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

Quaternion GetRotationToTheLocation(Vector3 Target, float Height, Vector3 MyEnemy) {
    return Quaternion::LookRotation((Target + Vector3(0, Height, 0)) - MyEnemy, Vector3(0, 1, 0));
}

// PLAYER TO ME WITH OFFSET LOGS ONLY
void PlayerToMe(uintptr_t localPlayer, uintptr_t enemyList) {
    WriteLog("[PLAYERTOME] ===== START =====");
    WriteLog("[PLAYERTOME] localPlayer=0x%lx enemyList=0x%lx", localPlayer, enemyList);
    
    if (localPlayer == 0 || enemyList == 0) {
        WriteLog("[PLAYERTOME] FAIL: Null pointers");
        return;
    }
    
    // OFFSET LOG: localRootBone at localPlayer + 0x668
    uintptr_t localRootBone = Read<uintptr_t>(localPlayer + 0x668);
    WriteLog("[PLAYERTOME] localRootBone +0x668 = 0x%lx", localRootBone);
    
    if (localRootBone == 0) {
        WriteLog("[PLAYERTOME] FAIL: localRootBone NULL");
        return;
    }
    
    // OFFSET LOG: localTransform at localRootBone + 0x10
    uintptr_t localTransform = Read<uintptr_t>(localRootBone + 0x10);
    WriteLog("[PLAYERTOME] localTransform +0x10 = 0x%lx", localTransform);
    
    if (localTransform == 0) {
        WriteLog("[PLAYERTOME] FAIL: localTransform NULL");
        return;
    }
    
    Vector3 localRootPosition = GetNodePosition(localTransform);
    WriteLog("[PLAYERTOME] localRootPosition: (%.2f, %.2f, %.2f)", 
             localRootPosition.X, localRootPosition.Y, localRootPosition.Z);
    
    // OFFSET LOG: enemyRootBone at enemyList + 0x668
    uintptr_t enemyRootBone = Read<uintptr_t>(enemyList + 0x668);
    WriteLog("[PLAYERTOME] enemyRootBone +0x668 = 0x%lx", enemyRootBone);
    
    if (enemyRootBone == 0) {
        WriteLog("[PLAYERTOME] FAIL: enemyRootBone NULL");
        return;
    }
    
    // OFFSET LOG: enemyTransform at enemyRootBone + 0x10
    uintptr_t enemyTransform = Read<uintptr_t>(enemyRootBone + 0x10);
    WriteLog("[PLAYERTOME] enemyTransform +0x10 = 0x%lx", enemyTransform);
    
    if (enemyTransform == 0) {
        WriteLog("[PLAYERTOME] FAIL: enemyTransform NULL");
        return;
    }
    
    // OFFSET LOG: enemyTransformObj at enemyTransform + 0x10
    uintptr_t enemyTransformObj = Read<uintptr_t>(enemyTransform + 0x10);
    WriteLog("[PLAYERTOME] enemyTransformObj +0x10 = 0x%lx", enemyTransformObj);
    
    if (enemyTransformObj == 0) {
        WriteLog("[PLAYERTOME] FAIL: enemyTransformObj NULL");
        return;
    }
    
    // OFFSET LOG: enemyMatrix at enemyTransformObj + 0x20
    uintptr_t enemyMatrix = Read<uintptr_t>(enemyTransformObj + 0x20);
    WriteLog("[PLAYERTOME] enemyMatrix +0x20 = 0x%lx", enemyMatrix);
    
    if (enemyMatrix != 0) {
        // OFFSET LOG: Write position to enemyMatrix + 0x80
        Write<Vector3>(enemyMatrix + 0x80, localRootPosition);
        WriteLog("[PLAYERTOME] SUCCESS: Wrote to enemyMatrix+0x80");
    } else {
        WriteLog("[PLAYERTOME] FAIL: enemyMatrix NULL");
    }
    WriteLog("[PLAYERTOME] ===== END =====");
}

bool get_IsKnocked(uintptr_t instance) {
    if (instance == 0) return false;
    auto deadState = Read<uintptr_t>(instance + 0x1FF0);
    if (deadState == 0) return false;
    int knockState = Read<int>(deadState + 0x80);
    return (knockState == 8);
}

void CreateDataList(Response& SendResponse) {
    if (Actived.activar) {
        float bestDistance = 99999;
        bool hasValidTarget = false;
        Vector3 bestTargetPos = Vector3::Zero();
        SendResponse.PlayerCount = 0;
        
        uintptr_t gameFacadeOffset = s_working_facade_offset != 0 ? s_working_facade_offset : 0xB4C41A0;
        auto GameFacade_c = Read<uintptr_t>(libAddress + gameFacadeOffset);
        
        if (GameFacade_c) {
            auto P2 = Read<uintptr_t>(GameFacade_c + 0xB8);
            if(P2) {
                auto BaseGame = Read<uintptr_t>(P2 + 0x8);
                if (BaseGame) {
                    auto m_Match = Read<uintptr_t>(BaseGame + 0x90);
                    if (m_Match) {
                        auto TimeSerive = Read<uintptr_t>(BaseGame + 0x20);
                        if (TimeSerive) {
                            Write<float>(TimeSerive + 0x2C, 0.03299999982f);
                            
                            auto MatchIsRunning = Read<int>(m_Match + 0xCC);
                            if(MatchIsRunning != 4 && MatchIsRunning != 0 && MatchIsRunning == 1) {
                                auto localPlayer = Read<uintptr_t>(m_Match + 0xD8);
                                
                                if (localPlayer != 0) {
                                    auto FollowCamera = Read<uintptr_t>(localPlayer + 0x30);
                                    if (FollowCamera != 0) {
                                        auto Camera = Read<uintptr_t>(FollowCamera + 0x30);
                                        if (Camera != 0) {
                                            auto IntPtrCam = Read<uintptr_t>(Camera + 0x10);
                                            if (IntPtrCam != 0) {
                                                uintptr_t matrixOffset = string2Offset(WRAPPER_MARCO("0x100"));
                                                auto matrix = Read<D3DMatrix>(IntPtrCam + matrixOffset);
                                                auto dictionary = Read<MonoDictionary *>(BaseGame + 0xC0);
                                                
                                                if (dictionary) {
                                                    int numValues = dictionary->getNumValues();
                                                    auto valuesPtr = dictionary->getValues();
                                                    auto LocalPosition = GetNodePosition(GetPlayerHeadTF(localPlayer));
                                                    
                                                    for (int x = 0; x < numValues; x++) {
                                                        uintptr_t entryAddr = valuesPtr + (x * 0x18);
                                                        auto enemyList = Read<uintptr_t>(entryAddr + 0x10);
                                                        
                                                        if (enemyList == 0 || enemyList < 0x100000000) continue;
                                                        if (enemyList == localPlayer) continue;
                                                        
                                                        auto isClientBot = Read<bool>(enemyList + 0x450);
                                                        if (isClientBot != 1 && isClientBot != 0) continue;
                                                        
                                                        auto AvatarManager = Read<uintptr_t>(enemyList + 0x718);
                                                        if (AvatarManager == 0) continue;
                                                        
                                                        auto UmaAvatarSimple = Read<uintptr_t>(AvatarManager + 0x128);
                                                        if (UmaAvatarSimple == 0) continue;
                                                        
                                                        auto IsVisible = Read<bool>(UmaAvatarSimple + 0x101);
                                                        if (IsVisible != 1) continue;
                                                        
                                                        auto UmaData = Read<uintptr_t>(UmaAvatarSimple + 0x28);
                                                        if (UmaData == 0) continue;
                                                        
                                                        if (Read<bool>(UmaData + 0x81) != 0) continue;
                                                        if (Read<bool>(enemyList + 0x7C) != 0) continue;
                                                        
                                                        bool isKnocked = get_IsKnocked(enemyList);
                                                        
                                                        auto EnemyPosition = GetNodePosition(GetPlayerHeadTF(enemyList));
                                                        auto EnemyPositionPe = GetNodePosition(GetPlayerPeTF(enemyList));
                                                        
                                                        if (EnemyPosition == Vector3::Zero()) continue;
                                                        
                                                        // PlayerToMe
                                                        if(pPlayer.PlayerToMe) {
                                                            PlayerToMe(localPlayer, enemyList);
                                                        }
                                                        
                                                        if (pPlayer.aimbotlock) {
                                                            auto HedColider = Read<uintptr_t>(enemyList + 0x6E0);
                                                            if (HedColider) Write(enemyList + 0x80, HedColider);
                                                        }
                                                        
                                                        auto CameraPosition = GetPosition(GetPlayerMainCamera(localPlayer));
                                                        float distanceToEnemy = Vector3::Distance(LocalPosition, EnemyPosition);
                                                        
                                                        auto LocationHeadBox = WorldToScreenPoint(matrix, EnemyPosition);
                                                        auto LocationToeBox = WorldToScreenPoint(matrix, EnemyPositionPe);
                                                        
                                                        if (LocationHeadBox.X < -0 || LocationToeBox.X < -0) continue;
                                                        
                                                        float screenDist = sqrt(pow(LocationHeadBox.X - g_screenWidth/2, 2) + 
                                                                               pow(LocationHeadBox.Y - g_screenHeight/2, 2));
                                                        
                                                        auto mIsFiring = Read<int>(localPlayer + 0x7E0);
                                                        
                                                        // AIMBOT
                                                        if(pPlayer.aimbot && screenDist < pPlayer.AimFov && mIsFiring > 0 && !isKnocked) {
                                                            float aimHeight = (pPlayer.aimbotType == 0) ? 0.0f : -0.18f;
                                                            Quaternion desiredRot = GetRotationToTheLocation(EnemyPosition, aimHeight, CameraPosition);  
                                                            Write<Quaternion>(localPlayer + 0x5BC, desiredRot);  
                                                        }
                                                        
                                                        // ORIGINAL SILENT AIM LOGIC - PRESERVED
                                                        // Find closest enemy for silent aim
                                                        if (distanceToEnemy < bestDistance && !isKnocked) {
                                                            bestDistance = distanceToEnemy;
                                                            bestTargetPos = EnemyPosition;
                                                            currentBestTargetObj = enemyList;
                                                            hasValidTarget = true;
                                                        }
                                                        
                                                        auto *data = &SendResponse.Players[SendResponse.PlayerCount];
                                                        data->Head = LocationHeadBox;
                                                        data->Toe = LocationToeBox;
                                                        data->IsCaido = isKnocked;
                                                        data->Distancia = distanceToEnemy;
                                                        
                                                        ++SendResponse.PlayerCount;
                                                        if (SendResponse.PlayerCount == maxplayerCount) break;
                                                    }
                                                    
                                                    // ORIGINAL SILENT AIM THREAD MANAGEMENT
                                                    bool shouldAim = hasValidTarget && silen4a;
                                                    if (shouldAim) {
                                                        currentBestTargetPos = bestTargetPos;
                                                        
                                                        if (!keepAiming) {
                                                            keepAiming = true;
                                                            
                                                            if (aimThread.joinable())  
                                                                aimThread.join();  
                                                            
                                                            aimThread = std::thread([&]() {  
                                                                ContinuousAim(localPlayer);  
                                                            });  
                                                        }
                                                    } else {
                                                        if (keepAiming) {
                                                            keepAiming = false;
                                                            
                                                            if (aimThread.joinable())  
                                                                aimThread.join();  
                                                            
                                                            currentBestTargetPos = Vector3::Zero();  
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void CheckGamePid() {
    if (target_pid > 0) {
        if (kill(target_pid, 0) != 0) {
            if (mem_fd > 0) {
                close(mem_fd);
                mem_fd = 0;
            }
            target_pid = 0;
            libAddress = 0;
        }
    }

    if (target_pid <= 0 || mem_fd <= 0) {
        target_pid = FindPid("com.dts.freefiremax");
        if (target_pid <= 0) {
            target_pid = FindPid("com.dts.freefireth");
        }
        if (target_pid > 0) {
            mem_fd = my_open_mem(target_pid);
            if (mem_fd > 0) {
                libAddress = FindCorrectLibBase();
            }
        }
    }
}

__attribute__ ((visibility ("hidden")))
int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    
    if (InitServer() != 0) {
        return 1;
    }

    while (true) {
        if (!server.Accept()) {
            usleep(200000);
            continue;
        }

        Request request{};
        while (server.receive((void*)&request) > 0) {
            Response SendResponse{};
            CheckGamePid();

            if (request.Mode == Mode::InitMode) {
                SendResponse.Success = true;
            } else if (request.Mode == Mode::HackMode) {
                SendResponse.Success = true;
            } else if (request.Mode == Mode::EspMode) {
                g_screenWidth = request.ScreenWidth;
                g_screenHeight = request.ScreenHeight;
                if (target_pid > 0 && mem_fd > 0 && libAddress > 0) {
                    CreateDataList(SendResponse);
                } else {
                    SendResponse.PlayerCount = 0;
                }
                SendResponse.Success = true;
            } else if (request.Mode == 3) {
                Actived.activar = request.m_IsOn;
                SendResponse.Success = true;
            } else if (request.Mode == 5) {
                pPlayer.aimbot = request.m_IsOn;
                SendResponse.Success = true;
            } else if (request.Mode == 22) {
                pPlayer.fovawm = request.m_IsOn;
                SendResponse.Success = true;
            } else if (request.Mode == 54) {
                pPlayer.aimbotlock = request.m_IsOn;
                SendResponse.Success = true;
            } else if (request.Mode == 55 || request.Mode == 57) {
                pPlayer.silen4a = request.m_IsOn;
                silen4a = request.m_IsOn;
                if (!silen4a) {
                    keepAiming = false;
                    if (aimThread.joinable())  
                        aimThread.join();
                    currentBestTargetPos = Vector3::Zero();
                }
                SendResponse.Success = true;
            } else if (request.Mode == 56) {
                pPlayer.PlayerToMe = request.m_IsOn;
                SendResponse.Success = true;
            } else if (request.Mode == 58) {
                pPlayer.AimFov = (float)request.value;
                SendResponse.Success = true;
            } else if (request.Mode == 59) {
                pPlayer.AimDistance = (float)request.value;
                SendResponse.Success = true;
            } else if (request.Mode == 60) {
                pPlayer.aimbotType = request.value;
                SendResponse.Success = true;
            } else {
                SendResponse.Success = true;
            }
            server.send((void*)&SendResponse, sizeof(SendResponse));
        }

        keepAiming = false;
        if (aimThread.joinable()) {
            aimThread.join();
        }
    }

    return 0;
}