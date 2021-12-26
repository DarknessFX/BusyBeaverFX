/// TODO: Visualization panel in ImGui

#pragma region Base references
#include <windows.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <format>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <chrono>

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include "imgui.h"
#include "backends\imgui_impl_win32.h"
#include "backends\imgui_impl_sdl.h"
#include "backends\imgui_impl_sdlrenderer.h"
#include "IconsFontAwesome5.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "SDL_syswm.h"
#pragma endregion


#pragma region LOG
/// 
/// LOG
/// 
struct log {
    log() { oLog << " \n \n \n \n.\n..\n...\n"; }
    ~log() { }
private:
    std::ostringstream oLog{};
    const int MAX_LOG_LINES = 50;
    void Trim() {
        std::vector<std::string> vTrim;
        std::istringstream ioLog{oLog.str()};
        for (std::string Line ; std::getline(ioLog, Line);) { vTrim.push_back(Line); }
        if (vTrim.size() > MAX_LOG_LINES) {
            oLog.str("");            
            for (int i=(int)vTrim.size() - MAX_LOG_LINES; i < vTrim.size(); ++i) { oLog << vTrim[i] << '\n'; }
        }
    }
public:
    std::string lastLine{""};
    template<typename ...T>
    void operator () (T&... Msg) {
        Trim();
        std::string _Msg{ std::format(Msg...) };
        oLog << _Msg << '\n';
        lastLine = _Msg;
    }
    std::string full() {
        return oLog.str();
    }
} LOG;
#pragma endregion


#pragma region Global memory and functions
/// 
/// Global memory and functions
/// 
struct Global {
    Global() { LOG("Starting..."); } 
    ~Global() { LOG("Closing..."); }

    const std::string AppName = "BusyBeaverFX";
    bool isClosing{false};
    bool isBusy{false};
    bool isCancel{false};
    std::string exePath{""};
    void pathFromArgv(char *argv) { 
        exePath = argv;
        exePath = exePath.substr(0, exePath.find_last_of("\\") + 1);
    }
    ImVec4 clear_color{ImVec4(0.0f, 0.0f, 0.0f, 1.00f)};

    struct Window {
        HWND hWnd{};
        HINSTANCE hInst{};
        HDC hDC{};
        int x{640};
        int y{360};
        ImVec2 getVec2() { return ImVec2((float)x, (float)y); };
        void UpdateSize(int w = 0, int h = 0) {
            x = w;
            y = h;
        }
        void loadIcon() {
            LOG("Loading icon...");
            HICON hIcon = LoadIcon(hInst, (LPWSTR)"IDI_ICON1");
            SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR) LoadIcon(hInst, (LPWSTR)"IDI_ICON1")); 
        }
    } win;

    void SaveToFile(std::string str, std::string Filename = "FSM_States.ini") {
        Filename.insert(0, exePath);
        std::fstream oFile{Filename, oFile.out };
        if (!oFile.is_open()) {
            LOG("Error => Failed to create save file at {}.", (Filename));
        } else {
            oFile.write(str.c_str(), str.size());
            oFile.close();
        }
    }
    std::string LoadFromFile(std::string Filename = "FSM_States.ini") {
        Filename.insert(0, exePath);
        if (std::filesystem::exists(Filename)) {
            std::fstream oFile{Filename, oFile.in };
            if (!oFile.is_open()) {
                LOG("Error => Failed to read save file at {}.", (Filename));
            } else {
                std::ostringstream ostr{};
                ostr << oFile.rdbuf();
                oFile.close();
                return ostr.str();
            }
        }
        return "";
    }
} global;
#pragma endregion


#pragma region FSM
std::vector<char> fsm_States{'A', 'B', 'C'};
enum class fsm_Inp: byte { Zero, One };
inline std::string enumToString(fsm_Inp v) {
    switch (v) {
        case fsm_Inp::Zero:  return "0";
        case fsm_Inp::One:   return "1";
        default:    return "";
    }
}
enum class fsm_Out: byte { Zero, One };
inline std::string enumToString(fsm_Out v) {
    switch (v) {
        case fsm_Out::Zero:  return "0";
        case fsm_Out::One:   return "1";
        default:    return "";
    }
}
enum fsm_Act: byte { Left, Right, Halt };
inline std::string enumToString(fsm_Act v) {
    switch (v) {
        case Left:  return "Left";
        case Right: return "Right";
        case Halt:  return "Halt";
        default:    return "";
    }
}
inline char enumToChar(fsm_Act v) {
    switch (v) {
        case Left:  return 'L';
        case Right: return 'R';
        case Halt:  return 'H';
        default:    return ' ';
    }
}
struct fsm {
    char State{};
    fsm_Inp Input{};
    fsm_Out Output{};
    fsm_Act Action{};
    char NextState{};
    std::string ToString() {
        std::string str{""};
        str += State;
        str += enumToString(Input);
        str += enumToString(Output);
        str += enumToChar(Action);
        str += NextState;
        return str;
    }
};
std::vector<fsm> FSM{
    { 'A', fsm_Inp::Zero, fsm_Out::One, Right, 'B' },
    { 'A', fsm_Inp::One,  fsm_Out::One, Left,  'C' },
    { 'B', fsm_Inp::Zero, fsm_Out::One, Left,  'A' },
    { 'B', fsm_Inp::One,  fsm_Out::One, Right, 'B' },
    { 'C', fsm_Inp::Zero, fsm_Out::One, Left,  'B' },
    { 'C', fsm_Inp::One,  fsm_Out::One, Halt,  'C' },
};
std::string FSMToString() {
    std::string str{""};
    for (fsm &_fsm : FSM) {
        str += _fsm.ToString() + ";";
    }
    return str;
}
void FSMFromString(std::string str) {
    if (str == "") { return; }
    fsm_States.clear();
    FSM.clear();
    std::istringstream instr{str};
    std::string _str{""};
    int iLine{0};
    while (std::getline(instr, _str, ';')) {
        char cState{(char)_str.substr(0,1).c_str()[0]};
        char cAction{(char)_str.substr(3,1).c_str()[0]};
        char cNextState{(char)_str.substr(4,1).c_str()[0]};
        if (iLine == 0 || iLine % 2 == 0) { fsm_States.push_back(cState); }
        fsm _fsm{};
        _fsm.State = cState;
        if (_str.substr(1,1) == "0") {
            _fsm.Input = fsm_Inp::Zero;
        } else {
            _fsm.Input = fsm_Inp::One;
        }
        if (_str.substr(2,1) == "0") {
            _fsm.Output = fsm_Out::Zero;
        } else {
            _fsm.Output = fsm_Out::One;
        }
        switch (cAction) {
            case 'L': _fsm.Action = fsm_Act::Left; break;
            case 'R': _fsm.Action = fsm_Act::Right; break;
            case 'H': _fsm.Action = fsm_Act::Halt; break;
        }
        _fsm.NextState = cNextState;
        FSM.push_back(_fsm);
        ++iLine;
    }
}
#pragma endregion


#pragma region Beaver
void beaver_Read();
void beaver_Write();
void beaver_CheckInfinity();
void beaver_Busy();
struct BEAVER {
    std::atomic_flag readingFlag{};
    std::atomic_flag writingFlag{};
    std::deque<byte> Tape{0};
    int cur_FSM{0};
    int cur_Tape{0};
    byte val_Tape{0};
    unsigned long long int Steps{0};
    int Score{0};
    std::chrono::steady_clock::time_point Time{std::chrono::high_resolution_clock::now()};
    std::ostringstream bLog{""};
    void Tape_Expand() {
        if (Tape.size() == 0) { Tape.push_front(0); cur_Tape = 1; }
        for (int i=0; i<10; ++i) { Tape.push_back(0); Tape.push_front(0); }
        cur_Tape += 10;
    }
    bool ValidateFSM() {
        if (FSM[0].NextState == 'A' &&
            FSM[0].Action != fsm_Act::Halt ) {
            LOG("Infinity => A0 with NextState A.");
            return false;
        }
        bool isHaltFound{false};
        bool isInvalidState{false};
        std::string _fsm_invState{};
        for (fsm &_fsm : FSM) {
            if (_fsm.Action == fsm_Act::Halt) { isHaltFound = true; continue; }
            if (std::find(fsm_States.begin(), fsm_States.end(), _fsm.NextState) == fsm_States.end()) { isInvalidState = true; _fsm_invState = _fsm.ToString(); break; }
        }
        if (isInvalidState) {
            LOG("Error => Invalid NextState found at {}.", _fsm_invState);
            return false;
        }
        if (!isHaltFound) {
            LOG("Infinity => No Halt Action found.");
            return false;
        }
        return true;
    }
    void Reset() {
        cur_FSM = 0;
        cur_Tape = 0;
        Steps = 1;
        Score = 0;
        bLog.str("");
        Tape.clear();
        Tape_Expand();
    }
    void Start() {
        global.isBusy = true;

        if (!ValidateFSM()) { 
            global.isBusy = false;
            return; 
        }
        global.SaveToFile(FSMToString());
        Reset();

        std::thread tRead(beaver_Read);
        std::thread tWrite(beaver_Write);
        std::thread tInf(beaver_CheckInfinity);
        std::thread tBusy(beaver_Busy);

        LOG("Started.");
        Time = std::chrono::high_resolution_clock::now();
        tRead.detach();
        tWrite.detach();
        tInf.detach();
        tBusy.detach();

    }
    void Stop() {
        global.isBusy = false;
        LOG("Stopped.");
    }
    void Halt() {
        global.isBusy = false;
        std::chrono::duration<double> diffTime = std::chrono::high_resolution_clock::now() - Time;
        double dTime = diffTime.count();
        std::string hMsg{std::format(std::locale("en_US.UTF-8"), "Halt. {:L} Steps. {:L} Score. {:.7f}s Time.", Steps, Score, dTime)};
        LOG(hMsg);
        
        // Save to file
        std::string str{FSMToString()};
        str += '\n';
        str += hMsg + '\n';
        str += bLog.str();
        std::string fName{std::format("{:%F_%T}.txt", std::chrono::system_clock::now())};
        std::replace(fName.begin(), fName.end(), ':', '-');
        global.SaveToFile(str, fName);
    }
} beaver;
void beaver_Read() {
    while (global.isBusy) {
        if (beaver.readingFlag.test()) {
            beaver.val_Tape = beaver.Tape[beaver.cur_Tape];
            beaver.readingFlag.wait(false);
            beaver.readingFlag.clear();
            beaver.readingFlag.notify_one();
        }
    }
}
void beaver_Write() {
    while (global.isBusy) {
        if (beaver.writingFlag.test()) {
            if (beaver.val_Tape == 0 && FSM[beaver.cur_FSM].Output == fsm_Out::One) {
                ++beaver.Score;
            }
            if (beaver.val_Tape == 1 && FSM[beaver.cur_FSM].Output == fsm_Out::Zero) {
                --beaver.Score;
            }
            beaver.Tape[beaver.cur_Tape] = (byte)FSM[beaver.cur_FSM].Output;
            beaver.writingFlag.wait(false);
            beaver.writingFlag.clear();
            beaver.writingFlag.notify_one();
        }
    }
}
void beaver_Busy() {
    while (global.isBusy) {
        beaver.readingFlag.test_and_set();
        beaver.readingFlag.wait(true);
        beaver.readingFlag.notify_one();
        if (beaver.val_Tape == 1) { ++beaver.cur_FSM; }
        beaver.writingFlag.test_and_set();
        beaver.writingFlag.wait(true);
        beaver.writingFlag.notify_one();
        beaver.bLog << FSM[beaver.cur_FSM].ToString() << ';';
        switch (FSM[beaver.cur_FSM].Action) {
            case Left:   --beaver.cur_Tape; break;
            case Right:  ++beaver.cur_Tape; break;
            case Halt:   beaver.Halt(); break;
        }
        beaver.cur_FSM = (FSM[beaver.cur_FSM].NextState - 65) * 2;
        ++beaver.Steps;
        if (beaver.cur_Tape == 0 ||
            beaver.cur_Tape == (int)beaver.Tape.size() - 1 ) { 
            beaver.Tape_Expand();
        }
    }
}
void beaver_CheckInfinity() {
    std::string state_History{""};
    std::string state_Current{""};
    int mem_FSM{0};
    unsigned long long int mem_Step{0};
    while (global.isBusy) {
        if (mem_FSM != beaver.cur_FSM || 
            mem_Step != beaver.Steps ) {
            mem_FSM = beaver.cur_FSM;
            mem_Step = beaver.Steps;
            if (beaver.Steps <= FSM.size()) {
                state_History += FSM[beaver.cur_FSM].ToString() + ";";
            } else {
                state_Current += FSM[beaver.cur_FSM].ToString() + ";";
            }
            if (beaver.Steps % FSM.size() == 0) {
                if (state_History != state_Current) {
                    state_History = state_Current;
                } else {
                    //Infinity Detected
                    beaver.Stop();
                    LOG("Infinity => Looping sequence {}.", state_Current);                    
                }
            }
        }
    }
}
#pragma endregion


#pragma region SDL2
/// 
/// SDL2
/// 
struct SDL {
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Event event;
    SDL_SysWMinfo windowinfo{};
    SDL_RendererInfo renderinfo{};

    SDL() { LOG("Start SDL2..."); }
    ~SDL() { LOG("Closing SDL2..."); }

    int Start() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
            std::cout << "Error: " << SDL_GetError() << "\n";
            return -1;
        }
        SDL_WindowFlags window_flags{(SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI)};
        window = SDL_CreateWindow(global.AppName.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, global.win.x, global.win.y, window_flags);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        if (renderer == NULL) {
            LOG("Error creating SDL_Renderer!");
            return -1;
        }
        SDL_GetWindowWMInfo(window, &windowinfo);
        global.win.hWnd = windowinfo.info.win.window;
        global.win.hInst = windowinfo.info.win.hinstance;
        global.win.hDC = windowinfo.info.win.hdc;
        SDL_VERSION(&windowinfo.version);
        LOG("SDL2 Version: {}.{}.{}", windowinfo.version.major, windowinfo.version.minor, windowinfo.version.patch);
        SDL_GetRendererInfo(renderer, &renderinfo);
        return 0;
    }

    void Event() {
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    global.isClosing = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE:
                            if (event.window.windowID == SDL_GetWindowID(sdl.window)) {
                                global.isClosing = true;
                            }
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                            global.win.UpdateSize(event.window.data1, event.window.data2);
                            break;
                    }
                case SDL_KEYDOWN:  // LSHIFT+ESC close window
                    if(event.key.keysym.sym == SDLK_ESCAPE) {
                        const Uint8 *keystate = SDL_GetKeyboardState(NULL);
                        if (keystate[SDL_SCANCODE_LSHIFT]) {
                            global.isClosing = true;
                        }
                    }
                    break;
            }
        }
    }

    void Render() {
        ImGui::Render();
        SDL_SetRenderDrawColor(sdl.renderer, (Uint8)(global.clear_color.x * 255), (Uint8)(global.clear_color.y * 255), (Uint8)(global.clear_color.z * 255), (Uint8)(global.clear_color.w * 255));
        SDL_RenderClear(sdl.renderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(sdl.renderer);
    }

    void Quit() {
        SDL_DestroyRenderer(sdl.renderer);
        SDL_DestroyWindow(sdl.window);
        SDL_Quit();
    }
} sdl;
#pragma endregion


#pragma region DearImGui
/// 
/// DearImGui
/// 
struct IMGUI {
    IMGUI() { LOG("Start ImGui..."); }
    ~IMGUI() { LOG("Closing ImGui..."); }

    void Start() {
        IMGUI_CHECKVERSION();
        LOG("ImGui : {}", IMGUI_VERSION);
        ImGui::CreateContext();
        ImGuiIO& io{ImGui::GetIO()}; (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplSDL2_InitForSDLRenderer(sdl.window);
        ImGui_ImplSDLRenderer_Init(sdl.renderer);

        ImFontConfig config{};
        config.MergeMode = true;
        config.GlyphMinAdvanceX = 13.0f; 
        config.PixelSnapH = true;
        static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        io.Fonts->AddFontFromFileTTF((global.exePath + "Roboto-Medium.ttf").c_str(), 13.0f);
        //io.Fonts->AddFontFromFileTTF((global.exePath + "fa-regular-400.ttf").c_str(), 13.0f, &config, icon_ranges);
        io.Fonts->AddFontFromFileTTF((global.exePath + "fa-solid-900.ttf").c_str(), 13.0f, &config, icon_ranges);
        io.Fonts->Build();
    }

    void NewFrame() {
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        Render();
    }

    bool show_log{false};
    void Render() {
        using namespace ImGui;
        {  //TableFSM
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
            PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(5, 5));
            SetNextWindowPos(ImVec2{ 0, 0 });
            SetNextWindowSize(ImVec2{ (float)global.win.x, GetTextLineHeightWithSpacing() * 12});
            Begin("FSM", NULL, 
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus);
            PopStyleVar(2);

            if (!global.isBusy) {
                PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.42f, 0.6f, 0.6f));
                PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.42f, 0.7f, 0.7f));
                PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.42f, 0.8f, 0.8f));
                if (SmallButton(ICON_FA_PLAY " Start")) {
                    beaver.Start();
                }
                PopStyleColor(3);
                SameLine(GetContentRegionMax().x - CalcTextSize("00000States00000").x);
                if (FSM.size() >= 52) { 
                    BeginDisabled(true); 
                    SmallButton(ICON_FA_PLUS);
                    EndDisabled(); 
                } else {
                    if (SmallButton(ICON_FA_PLUS)) {
                        char _LastState{FSM[FSM.size() - 1].State};
                        fsm_States.push_back(_LastState + 1);
                        fsm _NewState{ _LastState + 1, fsm_Inp::Zero, fsm_Out::One, Right, _LastState };
                        FSM.push_back(_NewState);
                        _NewState.Input = fsm_Inp::One;
                        _NewState.Action = Left;
                        FSM.push_back(_NewState);
                        LOG("States : Added {}.", FSM[FSM.size() - 1].State);
                    }
                }
                SameLine();
                Text("States"); 
                SameLine();
                if (FSM.size() <= 2) { 
                    BeginDisabled(true); 
                    SmallButton(ICON_FA_MINUS);
                    EndDisabled();
                } else {
                    if (SmallButton(ICON_FA_MINUS)) {
                        LOG("States : Removed {}.", FSM[FSM.size() - 1].State);
                        fsm_States.pop_back();
                        FSM.pop_back();
                        FSM.pop_back();
                    }
                }
            } else {
                PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
                PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
                PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
                if (SmallButton(ICON_FA_STOP " Stop")) {
                    beaver.Stop();
                }
                PopStyleColor(3);
            }

            static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | 
                                           ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | 
                                           ImGuiTableFlags_PreciseWidths;
            ImVec2 outer_size = ImVec2(0.0f, GetTextLineHeightWithSpacing() * 10);
            if (ImGui::BeginTable("tableFSM", 5, flags, outer_size))
            {
                TableSetupScrollFreeze(0, 1);
                TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, (global.win.x * 0.12f));
                TableSetupColumn("Input", ImGuiTableColumnFlags_WidthFixed, (global.win.x * 0.12f));
                TableSetupColumn("Output", ImGuiTableColumnFlags_WidthFixed, (global.win.x * 0.14f));
                TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, (global.win.x * 0.22f));
                TableSetupColumn( " Next\nState", ImGuiTableColumnFlags_WidthFixed, (global.win.x * 0.22f));

                TableNextRow(ImGuiTableRowFlags_Headers, GetTextLineHeightWithSpacing() * 1.5f);
                for (int i{0}; i<5; ++i) {
                    TableSetColumnIndex(i);
                    PushID(i);
                    SetCursorPosX(GetCursorPosX() + (GetColumnWidth() - CalcTextSize(TableGetColumnName(i)).x) / 2);
                    SetCursorPosY(GetCursorPosY() + (GetTextLineHeightWithSpacing() * 1.5f - CalcTextSize(TableGetColumnName(i)).y) / 2);
                    TableHeader(TableGetColumnName(i));
                    PopID();
                }

                ImGuiListClipper clipper;
                clipper.Begin((int)FSM.size());
                while (clipper.Step())
                {
                    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                    {
                        TableNextRow();
                        for (int i{0}; i<5; ++i) {
                            TableSetColumnIndex(i);
                            PushID(i);
                            std::string _text{};
                            switch (i) {
                                case 0: _text = FSM[row].State; break;
                                case 1: _text = enumToString(FSM[row].Input); break;
                                case 2: _text = enumToString(FSM[row].Output); break;
                                case 3: _text = enumToString(FSM[row].Action); break;
                                case 4: _text = FSM[row].NextState; break;
                            }
                            ImVec2 _size{CalcTextSize(_text.c_str())};
                            switch (i) {
                                case 0:
                                case 1:
                                    SetCursorPosY(GetCursorPosY() + (GetTextLineHeightWithSpacing() - _size.y) / 2);
                                    SetCursorPosX(GetCursorPosX() + (GetColumnWidth() - _size.x) / 2);
                                    Text(_text.c_str(), i, row);
                                    break;
                                case 2:
                                    if (!global.isBusy) {
                                        const char* items[] = { "0", "1" };
                                        int selected{(int)FSM[row].Action};
                                        PushID("comboOut" + row);
                                        PushItemWidth(global.win.x * 0.14f);
                                        if (BeginCombo("##_comboOut" + row, _text.c_str())) {
                                            for (int i{0}; i < 2; ++i) {
                                                if (items[i] == enumToString(FSM[row].Output)) { 
                                                    Selectable(items[i], true);
                                                    SetItemDefaultFocus();
                                                } else {
                                                    if (Selectable(items[i], false)) {
                                                        switch (i) {
                                                            case 0: FSM[row].Output = fsm_Out::Zero; break;
                                                            case 1: FSM[row].Output = fsm_Out::One; break;
                                                        }
                                                    }
                                                }
                                            }
                                            EndCombo();
                                        }
                                        PopID();
                                    } else {
                                        SetCursorPosX(GetCursorPosX() + (GetColumnWidth() - _size.x) / 2);
                                        Text(_text.c_str(), i, row);
                                    }
                                    break;
                                case 3: 
                                    if (!global.isBusy) {
                                        const char* items[] = { "Left", "Right", "Halt" };
                                        int selected{(int)FSM[row].Action};
                                        PushID("comboAct" + row);
                                        PushItemWidth(global.win.x * 0.22f);
                                        if (BeginCombo("##_comboAct" + row, _text.c_str())) {
                                            for (int i{0}; i < 3; ++i) {
                                                if (items[i] == enumToString(FSM[row].Action)) { 
                                                    Selectable(items[i], true);
                                                    SetItemDefaultFocus();
                                                } else {
                                                    if (Selectable(items[i], false)) {
                                                        switch (i) {
                                                            case 0: FSM[row].Action = fsm_Act::Left; break;
                                                            case 1: FSM[row].Action = fsm_Act::Right; break;
                                                            case 2: FSM[row].Action = fsm_Act::Halt; break;
                                                        }
                                                    }
                                                }
                                            }
                                            EndCombo();
                                        }
                                        PopID();
                                    } else {
                                        SetCursorPosX(GetCursorPosX() + (GetColumnWidth() - _size.x) / 2);
                                        Text(_text.c_str(), i, row);
                                    }
                                    break;
                                case 4:
                                    if (!global.isBusy) {
                                        PushID("comboNxt" + row);
                                        PushItemWidth(global.win.x * 0.22f);
                                        if (BeginCombo("##_comboNxt" + row, _text.c_str())) {
                                            for (int i{0}; i < fsm_States.size(); ++i) {
                                                std::string _NextState{fsm_States[i]};
                                                if (fsm_States[i] == FSM[row].NextState) { 
                                                    Selectable(_NextState.c_str(), true);
                                                    SetItemDefaultFocus();
                                                } else {
                                                    if (Selectable(_NextState.c_str(), false)) {
                                                        FSM[row].NextState = fsm_States[i];
                                                    }
                                                }
                                            }
                                            EndCombo();
                                        }
                                        PopID();
                                    } else {
                                        SetCursorPosX(GetCursorPosX() + (GetColumnWidth() - _size.x) / 2);
                                        Text(_text.c_str(), i, row);
                                    }
                                    break;
                            }
                            PopID();
                        }
                    }
                }
                EndTable();
            }
            End();
        }

        { //Statusbar
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
            PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(5, 5));
            SetNextWindowPos(ImVec2{ 0, global.win.getVec2().y - GetFrameHeightWithSpacing()});
            SetNextWindowSize(ImVec2{ global.win.getVec2().x, GetFrameHeightWithSpacing()});
            Begin("statusbar", NULL, 
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus);
            PopStyleVar(2);
            if (!show_log) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.14f, 0.27f, 0.42f, 1.0f });
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.06f, 0.53f, 0.98f, 1.0f });
            }
            if (SmallButton(ICON_FA_FILE_ALT " Log")) { show_log = !show_log; };
            PopStyleColor(1);
            SameLine();
            if (!global.isBusy) {
                Text(LOG.lastLine.c_str()); 
            } else {
                Text(std::format(std::locale("en_US.UTF-8"), "Steps : {:L}", beaver.Steps).c_str()); 
            }
            SameLine(GetContentRegionMax().x - CalcTextSize("FPS 00.0 (00.000)0000").x);
            Text(ICON_FA_EYE " FPS %.1f (%.3f)", GetIO().Framerate, 1000.0f / GetIO().Framerate);
            End();

            if (show_log) {
                PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
                PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(5, 5));
                SetNextWindowPos(ImVec2{ 0, global.win.getVec2().y - (GetFrameHeightWithSpacing() * 4)});
                SetNextWindowSize(ImVec2{ global.win.getVec2().x, (GetFrameHeightWithSpacing() * 3)});
                Begin("log_window", NULL, 
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus);
                PopStyleVar(2);
                TextUnformatted(LOG.full().c_str());
                if ( GetScrollY() == 0.0f ) { 
                    SetScrollHereY(1.0f); 
                }
                End();
            }
        }
    }

    void Quit() {
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }
} imgui;
#pragma endregion


#pragma region Main function
/// 
/// Main
/// 
int main(int argc, char *argv[])
{
    global.pathFromArgv(argv[0]);
    FSMFromString(global.LoadFromFile());
    sdl.Start();
    imgui.Start();
    global.win.loadIcon();

    LOG("Ready.");
    while (!global.isClosing)
    {
        sdl.Event();
        imgui.NewFrame();
        sdl.Render();
    }

    LOG("Closing...");
    imgui.Quit();
    sdl.Quit();

    LOG("End.");
    return 0;
}
#pragma endregion