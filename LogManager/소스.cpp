#include <iostream>
#include <iomanip>
#include <cctype>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <io.h>
#include <fcntl.h>

// 헥스 덤프 함수
void HexDump(const std::wstring& data) {
    const wchar_t* byteData = data.c_str();
    const size_t size = data.size() * sizeof(wchar_t); // 총 데이터 크기 (바이트 단위)
    const size_t bytesPerLine = 16; // 한 줄에 출력할 바이트 수

    const unsigned char* bytePtr = reinterpret_cast<const unsigned char*>(byteData);

    for (size_t i = 0; i < size; i += bytesPerLine) {
        // 주소 출력
        std::cout << std::setw(8) << std::setfill('0') << std::hex << i << ": ";

        // 16진수 데이터 출력
        for (size_t j = 0; j < bytesPerLine; ++j) {
            if (i + j < size)
                std::cout << std::setw(2) << static_cast<int>(bytePtr[i + j]) << " ";
            else
                std::cout << "   ";
        }

        // ASCII 출력 (wchar_t는 범위가 크기 때문에 제한적으로만 처리)
        std::cout << " | ";
        for (size_t j = 0; j < bytesPerLine; ++j) {
            if (i + j < size) {
                unsigned char ch = bytePtr[i + j];
                std::cout << (std::isprint(ch) ? static_cast<char>(ch) : '.');
            }
            else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
}

// 헥스 스트링을 wstring으로 변환하는 함수
std::wstring HexToWString(const std::vector<std::string>& hexStrings) {
    std::vector<unsigned char> bytes;

    for (const auto& hex : hexStrings) {
        try {
            unsigned char byte = static_cast<unsigned char>(std::stoi(hex, nullptr, 16));
            bytes.push_back(byte);
        }
        catch (const std::invalid_argument& e) {
            std::cerr << "Invalid hex string: " << hex << std::endl;
            continue;
        }
    }

    return std::wstring(reinterpret_cast<const wchar_t*>(bytes.data()), bytes.size() / sizeof(wchar_t));
}

// 테스트를 위한 헥스 데이터 추출 및 변환
std::vector<std::string> ExtractHexFromHexDump(const std::string& hexDump) {
    std::istringstream stream(hexDump);
    std::string line;
    std::vector<std::string> hexValues;

    while (std::getline(stream, line)) {
        std::istringstream lineStream(line);
        std::string address;
        lineStream >> address; // 주소 부분 무시

        for (int i = 0; i < 16; ++i) {
            std::string hexValue;
            lineStream >> hexValue;
            if (hexValue.size() == 2) {
                hexValues.push_back(hexValue);
            }
        }
    }

    return hexValues;
}

int main() {
    // 콘솔 출력 모드를 유니코드로 설정
    _setmode(_fileno(stdout), _O_U16TEXT);

    std::wstring data = L"Hello, 헥스 덤프!";

    // 헥스 덤프 출력
    HexDump(data);

    //// 헥스 덤프를 텍스트로 가져옴
    //std::string hexDump = hexDumpStream.str();

    //// 헥스 값 추출
    //std::vector<std::string> hexValues = ExtractHexFromHexDump(hexDump);

    //// 헥스를 다시 wstring으로 변환
    //std::wstring restoredData = HexToWString(hexValues);

    //std::wcout << L"Original Data: " << data << std::endl;
    //std::wcout << L"Restored Data: " << restoredData << std::endl;

    return 0;
}

