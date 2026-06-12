#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>

// JSON 배열 "[{...},{...}]" 을 개별 "{...}" 문자열 목록으로 분리
inline std::vector<std::string> jsonSplitObjects(const std::string& json)
{
	std::vector<std::string> result;
	int depth = 0;
	size_t start = 0;
	for (size_t i = 0; i < json.size(); ++i) {
		if (json[i] == '{') {
			if (depth == 0) start = i;
			++depth;
		} else if (json[i] == '}') {
			--depth;
			if (depth == 0)
				result.push_back(json.substr(start, i - start + 1));
		}
	}
	return result;
}

// 파일 전체를 문자열로 읽기, 없거나 비어 있으면 "[]" 반환
inline std::string jsonReadFile(const std::string& path)
{
	std::ifstream f(path);
	if (!f.is_open()) return "[]";
	std::ostringstream ss;
	ss << f.rdbuf();
	std::string content = ss.str();
	return content.empty() ? "[]" : content;
}

// 문자열을 파일에 쓰기
inline void jsonWriteFile(const std::string& path, const std::string& content)
{
	std::ofstream f(path);
	if (!f.is_open())
		throw std::runtime_error("파일을 열 수 없습니다: " + path);
	f << content;
}

// JSON 객체 문자열에서 문자열 값 추출: "key":"value"
inline std::string jsonGetStr(const std::string& obj, const std::string& key)
{
	auto k = obj.find("\"" + key + "\":");
	if (k == std::string::npos) return "";
	size_t afterColon = k + key.size() + 3;	// '"key":' 다음 위치
	size_t q1 = obj.find('"', afterColon);
	if (q1 == std::string::npos) return "";
	size_t q2 = obj.find('"', q1 + 1);
	if (q2 == std::string::npos) return "";
	return obj.substr(q1 + 1, q2 - q1 - 1);
}

// JSON 객체 문자열에서 double 값 추출: "key":number
inline double jsonGetDbl(const std::string& obj, const std::string& key)
{
	auto k = obj.find("\"" + key + "\":");
	if (k == std::string::npos) return 0.0;
	size_t s = k + key.size() + 3;
	while (s < obj.size() && (obj[s] == ' ' || obj[s] == '\t' || obj[s] == '\n' || obj[s] == '\r')) ++s;
	try { return std::stod(obj.substr(s)); } catch (...) { return 0.0; }
}

// JSON 객체 문자열에서 int 값 추출
inline int jsonGetInt(const std::string& obj, const std::string& key)
{
	auto k = obj.find("\"" + key + "\":");
	if (k == std::string::npos) return 0;
	size_t s = k + key.size() + 3;
	while (s < obj.size() && (obj[s] == ' ' || obj[s] == '\t' || obj[s] == '\n' || obj[s] == '\r')) ++s;
	try { return std::stoi(obj.substr(s)); } catch (...) { return 0; }
}

// JSON 객체 문자열에서 long long 값 추출
inline long long jsonGetLong(const std::string& obj, const std::string& key)
{
	auto k = obj.find("\"" + key + "\":");
	if (k == std::string::npos) return 0LL;
	size_t s = k + key.size() + 3;
	while (s < obj.size() && (obj[s] == ' ' || obj[s] == '\t' || obj[s] == '\n' || obj[s] == '\r')) ++s;
	try { return std::stoll(obj.substr(s)); } catch (...) { return 0LL; }
}
