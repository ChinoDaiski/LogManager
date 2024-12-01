#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <mutex>
#include <iomanip>
#include <cstdarg>
#include <filesystem>
#include <vector>
#include <strsafe.h>

#include <unordered_map>
#include <Windows.h>
#include <bitset>


enum class LogLevel {
    LEVEL_DEBUG,
    LEVEL_ERROR,
    LEVEL_SYSTEM
};




class SystemLogManager {
public:
    static SystemLogManager& GetInstance() {
        static SystemLogManager instance;
        return instance;
    }
    ~SystemLogManager(void) {

        for (auto& iter : fileLockMap)
        {
            LeaveCriticalSection(&iter.second);
        }

        fileLockMap.clear();
    }

    void Initialize(const std::wstring& directory, LogLevel level) {
        InitializeDirectory(directory);
        InitializeLevel(level);
    }

    void InitializeDirectory(const std::wstring& directory)
    {
        logDirectory = directory;

        // 인자로 받은 파일 경로가 있는지 확인
        if (!std::filesystem::exists(logDirectory)) {
            // 없다면 해당 경로를 생성
            std::filesystem::create_directory(logDirectory);
        }
    }

    void InitializeLevel(LogLevel level)
    {
        logLevel = level;
    }



    void Log(const std::wstring& type, LogLevel level, const wchar_t* format, ...) 
    {
        if (level < logLevel) {
            return;
        }

        // type(파일명)을 key로 가진 CS가 있는지 확인한다. 
        auto iter = fileLockMap.find(type);

        // 만약 없다면
        if (iter == fileLockMap.end())
        {
            // CS를 추가한다.
            CRITICAL_SECTION cs;
            InitializeCriticalSection(&cs);

            fileLockMap.emplace(type, cs);

            iter = fileLockMap.find(type);
        }

        wchar_t logMessage[512]{ 0 };
        va_list args;
        va_start(args, format);

        // StringCchVPrintf를 사용하여 형식화된 메시지 작성
        int size = sizeof(logMessage);
        HRESULT hResult = StringCchVPrintf(logMessage, sizeof(logMessage) / sizeof(wchar_t), format, args);

        va_end(args); 
        
        if (FAILED(hResult))
        {
            // 실패한 경우 적절한 오류 처리
            wprintf(L"[LOG ERROR] String formatting failed. Error: 0x%08X\n", hResult);
            return;
        }

        auto now = std::time(nullptr);
        std::tm localTime;
        localtime_s(&localTime, &now);

        INT64 index = InterlockedIncrement64(&logIndex);

        std::wstringstream logLine;
        logLine 
            << L"[" << type << L"] [" << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S")
            << L" / " << LogLevelToString(level)
            << L" / " << std::setw(9) << std::setfill(L'0') << index
            << L"] " << logMessage << L"\n";

        // 콘솔에 출력
        std::wcout << logLine.str();

        // 파일에 기록
        std::wstring fileName = GetLogFileName(type);





        EnterCriticalSection(&iter->second);

        std::wofstream logFile(fileName, std::ios::app);
        if (logFile.is_open()) {
            logFile << logLine.str();
            logFile.close();
        }

        LeaveCriticalSection(&iter->second);
    }

    void LogHex(const std::wstring& type, LogLevel level, const std::wstring& description, const char* data, size_t length) {
        if (level < logLevel) {
            return;
        }

        auto now = std::time(nullptr);
        std::tm localTime;
        localtime_s(&localTime, &now);

        std::wstringstream logLine;
        logLine << L"[" << type << L"] [" << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S") << L" / " << LogLevelToString(level) << L" ] " << description << "\n";




        //for (size_t i = 0; i < length; ++i) {
        //    if (i % 16 == 0) {
        //        logLine << L"\n";
        //    }
        //    logLine << std::hex << std::setw(2) << std::setfill(L'0') << static_cast<int>(data[i]) << L" ";
        //}
        //logLine << L"\n";
 

        const size_t size = length * sizeof(char); // 총 데이터 크기 (바이트 단위)
        const size_t bytesPerLine = 16; // 한 줄에 출력할 바이트 수

        for (size_t i = 0; i < size; i += bytesPerLine) {
            // 주소 출력
            logLine << std::setw(8) << std::setfill(L'0') << std::hex << i << L": ";

            // 16진수 데이터 출력
            for (size_t j = 0; j < bytesPerLine; ++j) {
                if (i + j < size)
                    logLine << std::setw(2) << static_cast<int>(data[i + j]) << L" ";
                else
                    logLine << L"   ";
            }

            // ASCII 출력 (wchar_t는 범위가 크기 때문에 제한적으로만 처리)
            logLine << L" | ";
            for (size_t j = 0; j < bytesPerLine; ++j) {
                if (i + j < size) {
                    unsigned char ch = data[i + j];
                    logLine << (std::isprint(ch) ? static_cast<char>(ch) : '.');
                }
                else {
                    logLine << " ";
                }
            }
            logLine << std::endl;
        }





        std::wcout << logLine.str(); // Console output

        std::wstring fileName = GetLogFileName(type);
        std::wofstream logFile(fileName, std::ios::app);
        if (logFile.is_open()) {
            logFile << logLine.str();
            logFile.close();
        }









        

        //std::wcout << wss.str();

        //return wss.str();












    }

private:
    std::wstring logDirectory;  // 로그가 위치한 경로
    LogLevel logLevel;          // 로그 레벨
    INT64 logIndex = 0;        // 로그를 기록할 때 마다 1씩 증가하는 값. 이로서 모든 로그가 순서대로 찍힐 수 있음.
    std::unordered_map<std::wstring, CRITICAL_SECTION> fileLockMap;





    SystemLogManager() : logLevel(LogLevel::LEVEL_DEBUG) {}

    std::wstring GetLogFileName(const std::wstring& type) {
        auto now = std::time(nullptr);
        std::tm localTime;
        localtime_s(&localTime, &now);

        std::wstringstream fileName;
        fileName << logDirectory << L"/" << std::put_time(&localTime, L"%Y%m") << L"_" << type << L".txt";
        // 만약 일가지 넣고 싶다면 std::put_time(localTime, L"%Y%m%d")를 사용한다. 
        // 현재 시간 (localTime)을 "YYYYMMDD" 형식으로 출력
        // 예를 들어, 2024년 12월 1일이라면 "20241201"로 출력된다.

        return fileName.str();
    }

    std::wstring LogLevelToString(LogLevel level) const {
        switch (level) {
        case LogLevel::LEVEL_DEBUG: return L"DEBUG";
        case LogLevel::LEVEL_ERROR: return L"ERROR";
        case LogLevel::LEVEL_SYSTEM: return L"SYSTEM";
        default: return L"UNKNOWN";
        }
    }
};


struct GameLog {
    std::string server;     // 서버 이름
    std::string type;       // 컨텐츠. 같은 code 번호라도 현재 유저가 위치한 컨텐츠에 따라 다른 행동이 될 수 있다. BATTLE, CASH_SHOP... 등등
    std::string code;       // 행동, 예를 들어서 돈 획득 관련으로 [ 몬스터를 잡아서 돈 획득 ], [ 상점에 아이템을 팔아서 돈 획득 ], [ 유저 간 거래를 통한 돈 획득 ] 
    uint64_t accountNo;     // 계정 번호, 어떤 유저가 대상인지에 대해 저장.
    int32_t param1;         // 행동에 따른 추가적인 정보들 1 ~ 4, 필요할시 더 추가하는데 보통 4개면 충분. 적게 쓰지 더 많이 쓰이면 추가
    int32_t param2;
    int32_t param3;
    int32_t param4;
    std::string paramStr;   // 특정 수치로 나타내기 힘든 것은 문자열로 표현

    GameLog(const std::string& server, const std::string& type, const std::string& code,
        uint64_t accountNo, int32_t param1, int32_t param2, int32_t param3, int32_t param4,
        const std::string& paramStr)
        : server(server), type(type), code(code), accountNo(accountNo), param1(param1),
        param2(param2), param3(param3), param4(param4), paramStr(paramStr) {}
};

class GameLogManager {
public:
    /*
        Code : 몬스터 잡아서 돈 획득 / Param1 : 잡은 몬스터 / Param2: 1000원 획득 /  Param3: 총 231000원 보유

	    예시와 같이 어떤 행위를 통해서 무엇을 하였고, 무엇을 얻었고, 어떻게 되었는지를 아주아주 상세히 기록하여
	    로그만 보더라도 이 유저가 무엇을 했는지, 무엇을 얻었는지, 출처는 어디인지를 모두 확인할 수 있어야 한다.
    */
    static void SaveToDatabase(const GameLog& log) {
        // DB 저장 로직은 게임 개발 환경에 맞게 작성
        std::cout << "GameLog Saved: " << log.server << ", " << log.type << ", " << log.code
            << ", AccountNo: " << log.accountNo << ", Params: (" << log.param1 << ", "
            << log.param2 << ", " << log.param3 << ", " << log.param4 << "), Str: "
            << log.paramStr << "\n";
    }
};

// 매크로 정의
#define SYSLOG_DIRECTORY(dir)  SystemLogManager::GetInstance().InitializeDirectory(dir)
#define SYSLOG_LEVEL(level)  SystemLogManager::GetInstance().InitializeLevel(level)
#define LOG(type, level, format)  SystemLogManager::GetInstance().Log(type, level, format)
//#define LOG_HEX(type, level, format)  SystemLogManager::GetInstance().LogHex(type, level, format,)



int main() {
    // 시스템 로그 초기화
    //SystemLogManager::GetInstance().Initialize(L"Logs", LogLevel::LEVEL_DEBUG);
    SYSLOG_DIRECTORY(L"Logs");              // 로그를 저장 할 폴더 지정
    SYSLOG_LEVEL(LogLevel::LEVEL_DEBUG);    // 로그 레벨 지정

    // 시스템 로그 출력
    LOG(L"System", LogLevel::LEVEL_DEBUG, L"System initialized.");
    LOG(L"System", LogLevel::LEVEL_ERROR, L"Hello, %s! Your score is %d.", L"Player1", 100);

    //SystemLogManager::GetInstance().Log(L"System", LogLevel::LEVEL_DEBUG, L"System initialized.");
    //SystemLogManager::GetInstance().Log(L"System", LogLevel::LEVEL_ERROR, L"An error occurred.");
    //SystemLogManager::GetInstance().Log(L"System", LogLevel::LEVEL_ERROR, L"Hello, %s! Your score is %d.", L"Player1", 100);

    // 바이너리 로그 출력
    char sampleData[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };


    std::wstring data = L"Hello, 헥스 덤프!";
    SystemLogManager::GetInstance().LogHex(L"Memory", LogLevel::LEVEL_DEBUG, L"Sample binary data", sampleData, sizeof(sampleData));

    // 게임 로그 저장
    GameLog log("Server1", "Battle", "MonsterKilled", 123456, 1001, 2000, 500, 2500, "MonsterType: Dragon");
    GameLogManager::SaveToDatabase(log);

    return 0;
}
