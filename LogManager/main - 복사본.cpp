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

        // ���ڷ� ���� ���� ��ΰ� �ִ��� Ȯ��
        if (!std::filesystem::exists(logDirectory)) {
            // ���ٸ� �ش� ��θ� ����
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

        // type(���ϸ�)�� key�� ���� CS�� �ִ��� Ȯ���Ѵ�. 
        auto iter = fileLockMap.find(type);

        // ���� ���ٸ�
        if (iter == fileLockMap.end())
        {
            // CS�� �߰��Ѵ�.
            CRITICAL_SECTION cs;
            InitializeCriticalSection(&cs);

            fileLockMap.emplace(type, cs);

            iter = fileLockMap.find(type);
        }

        wchar_t logMessage[512]{ 0 };
        va_list args;
        va_start(args, format);

        // StringCchVPrintf�� ����Ͽ� ����ȭ�� �޽��� �ۼ�
        int size = sizeof(logMessage);
        HRESULT hResult = StringCchVPrintf(logMessage, sizeof(logMessage) / sizeof(wchar_t), format, args);

        va_end(args); 
        
        if (FAILED(hResult))
        {
            // ������ ��� ������ ���� ó��
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

        // �ֿܼ� ���
        std::wcout << logLine.str();

        // ���Ͽ� ���
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
 

        const size_t size = length * sizeof(char); // �� ������ ũ�� (����Ʈ ����)
        const size_t bytesPerLine = 16; // �� �ٿ� ����� ����Ʈ ��

        for (size_t i = 0; i < size; i += bytesPerLine) {
            // �ּ� ���
            logLine << std::setw(8) << std::setfill(L'0') << std::hex << i << L": ";

            // 16���� ������ ���
            for (size_t j = 0; j < bytesPerLine; ++j) {
                if (i + j < size)
                    logLine << std::setw(2) << static_cast<int>(data[i + j]) << L" ";
                else
                    logLine << L"   ";
            }

            // ASCII ��� (wchar_t�� ������ ũ�� ������ ���������θ� ó��)
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
    std::wstring logDirectory;  // �αװ� ��ġ�� ���
    LogLevel logLevel;          // �α� ����
    INT64 logIndex = 0;        // �α׸� ����� �� ���� 1�� �����ϴ� ��. �̷μ� ��� �αװ� ������� ���� �� ����.
    std::unordered_map<std::wstring, CRITICAL_SECTION> fileLockMap;





    SystemLogManager() : logLevel(LogLevel::LEVEL_DEBUG) {}

    std::wstring GetLogFileName(const std::wstring& type) {
        auto now = std::time(nullptr);
        std::tm localTime;
        localtime_s(&localTime, &now);

        std::wstringstream fileName;
        fileName << logDirectory << L"/" << std::put_time(&localTime, L"%Y%m") << L"_" << type << L".txt";
        // ���� �ϰ��� �ְ� �ʹٸ� std::put_time(localTime, L"%Y%m%d")�� ����Ѵ�. 
        // ���� �ð� (localTime)�� "YYYYMMDD" �������� ���
        // ���� ���, 2024�� 12�� 1���̶�� "20241201"�� ��µȴ�.

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
    std::string server;     // ���� �̸�
    std::string type;       // ������. ���� code ��ȣ�� ���� ������ ��ġ�� �������� ���� �ٸ� �ൿ�� �� �� �ִ�. BATTLE, CASH_SHOP... ���
    std::string code;       // �ൿ, ���� �� �� ȹ�� �������� [ ���͸� ��Ƽ� �� ȹ�� ], [ ������ �������� �ȾƼ� �� ȹ�� ], [ ���� �� �ŷ��� ���� �� ȹ�� ] 
    uint64_t accountNo;     // ���� ��ȣ, � ������ ��������� ���� ����.
    int32_t param1;         // �ൿ�� ���� �߰����� ������ 1 ~ 4, �ʿ��ҽ� �� �߰��ϴµ� ���� 4���� ���. ���� ���� �� ���� ���̸� �߰�
    int32_t param2;
    int32_t param3;
    int32_t param4;
    std::string paramStr;   // Ư�� ��ġ�� ��Ÿ���� ���� ���� ���ڿ��� ǥ��

    GameLog(const std::string& server, const std::string& type, const std::string& code,
        uint64_t accountNo, int32_t param1, int32_t param2, int32_t param3, int32_t param4,
        const std::string& paramStr)
        : server(server), type(type), code(code), accountNo(accountNo), param1(param1),
        param2(param2), param3(param3), param4(param4), paramStr(paramStr) {}
};

class GameLogManager {
public:
    /*
        Code : ���� ��Ƽ� �� ȹ�� / Param1 : ���� ���� / Param2: 1000�� ȹ�� /  Param3: �� 231000�� ����

	    ���ÿ� ���� � ������ ���ؼ� ������ �Ͽ���, ������ �����, ��� �Ǿ������� ���־��� ���� ����Ͽ�
	    �α׸� ������ �� ������ ������ �ߴ���, ������ �������, ��ó�� ��������� ��� Ȯ���� �� �־�� �Ѵ�.
    */
    static void SaveToDatabase(const GameLog& log) {
        // DB ���� ������ ���� ���� ȯ�濡 �°� �ۼ�
        std::cout << "GameLog Saved: " << log.server << ", " << log.type << ", " << log.code
            << ", AccountNo: " << log.accountNo << ", Params: (" << log.param1 << ", "
            << log.param2 << ", " << log.param3 << ", " << log.param4 << "), Str: "
            << log.paramStr << "\n";
    }
};

// ��ũ�� ����
#define SYSLOG_DIRECTORY(dir)  SystemLogManager::GetInstance().InitializeDirectory(dir)
#define SYSLOG_LEVEL(level)  SystemLogManager::GetInstance().InitializeLevel(level)
#define LOG(type, level, format)  SystemLogManager::GetInstance().Log(type, level, format)
//#define LOG_HEX(type, level, format)  SystemLogManager::GetInstance().LogHex(type, level, format,)



int main() {
    // �ý��� �α� �ʱ�ȭ
    //SystemLogManager::GetInstance().Initialize(L"Logs", LogLevel::LEVEL_DEBUG);
    SYSLOG_DIRECTORY(L"Logs");              // �α׸� ���� �� ���� ����
    SYSLOG_LEVEL(LogLevel::LEVEL_DEBUG);    // �α� ���� ����

    // �ý��� �α� ���
    LOG(L"System", LogLevel::LEVEL_DEBUG, L"System initialized.");
    LOG(L"System", LogLevel::LEVEL_ERROR, L"Hello, %s! Your score is %d.", L"Player1", 100);

    //SystemLogManager::GetInstance().Log(L"System", LogLevel::LEVEL_DEBUG, L"System initialized.");
    //SystemLogManager::GetInstance().Log(L"System", LogLevel::LEVEL_ERROR, L"An error occurred.");
    //SystemLogManager::GetInstance().Log(L"System", LogLevel::LEVEL_ERROR, L"Hello, %s! Your score is %d.", L"Player1", 100);

    // ���̳ʸ� �α� ���
    char sampleData[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };


    std::wstring data = L"Hello, �� ����!";
    SystemLogManager::GetInstance().LogHex(L"Memory", LogLevel::LEVEL_DEBUG, L"Sample binary data", sampleData, sizeof(sampleData));

    // ���� �α� ����
    GameLog log("Server1", "Battle", "MonsterKilled", 123456, 1001, 2000, 500, 2500, "MonsterType: Dragon");
    GameLogManager::SaveToDatabase(log);

    return 0;
}
