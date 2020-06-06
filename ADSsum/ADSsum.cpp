#include "stdafx.h"

wstring s2ws(const std::string& str)
{
	using convert_typeX = codecvt_utf8<wchar_t>;
	wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

string ws2s(const std::wstring& wstr)
{
	using convert_typeX = codecvt_utf8<wchar_t>;
	wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

char* readFileBytes(const char *name, int &outlen)
{
	ifstream fl(name);
	fl.seekg(0, ios::end);
	streamoff len = fl.tellg();
	char *ret = new char[(int) len];
	fl.seekg(0, ios::beg);
	fl.read(ret, len);
	fl.close();
	outlen = len;
	return ret;
}

BOOL FileExists(LPCWSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void getFileContents(string name, string &output)
{
	ifstream file(name, ifstream::binary);
	string temp, tempAll;
	while (getline(file, temp))
	{
		tempAll += temp;
		if (tempAll.back() == '\r')	//getline removes the \n, and we remove the \r
			tempAll.pop_back();
	}
	output = string(tempAll);
}

string sha256(const string str)
{
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, str.c_str(), str.size());
	SHA256_Final(hash, &sha256);
	stringstream ss;
	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		ss << hex << setw(2) << setfill('0') << (int)hash[i];
	return ss.str();
}

string GetOSSLSHA256Hash(void* content, int length)
{
	unsigned char digest[SHA256_DIGEST_LENGTH];
	char sha256String[65];

	SHA256((unsigned char*)content, length, (unsigned char*)&digest);

	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
         sprintf_s(&sha256String[i*2], 3, "%02x", (unsigned int)digest[i]);

	string digestString;
	digestString.append(sha256String);

	return digestString;
}

wstring GetOSSLSHA256HashW(void* content, int length)
{
	WCHAR digest[SHA256_DIGEST_LENGTH];
	WCHAR sha256String[65];

	SHA256((unsigned char*)content, length, (unsigned char*)&digest);

	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
		wsprintf(&sha256String[i * 2], L"%02x", (unsigned int)digest[i]);

	wstring digestString;
	digestString.append(sha256String);

	return digestString;
}

int ADSs(LPCWSTR lpFileName, WCHAR** &streamNamesOut, int &sizeOut)
{
	if (!FileExists(lpFileName))
		return 1;
	int basenameSize = lstrlenW(lpFileName);

	HANDLE hFindStream;
	STREAM_INFO_LEVELS InfoLevel = FindStreamInfoStandard;
	WIN32_FIND_STREAM_DATA findStreamData;
	DWORD dwFlags = 0;

	hFindStream = FindFirstStreamW(lpFileName, InfoLevel, &findStreamData, dwFlags);
	
	int count = 0;							//counts streams
	WCHAR** tempSNs = NULL;					//temp array in case we need to increase streamNames
	WCHAR** streamNames = new WCHAR*;		//streamNames intialized to 1
	if (hFindStream != INVALID_HANDLE_VALUE)
	{
		BOOL output;
		do 
		{
			//Get size and add to 'names'
			int streamSize = lstrlenW(findStreamData.cStreamName);
			WCHAR* nameBuf = new WCHAR[basenameSize+streamSize];	//temporary stream name buffer
			//copy basename to name buffer
			wchar_t * ltp = wmemcpy(nameBuf, lpFileName, basenameSize);
			if (!ltp) 
				return 2;
			//copy stream name to rest of name buffer
			ltp = wmemcpy(nameBuf+basenameSize, findStreamData.cStreamName, (streamSize));
			if (!ltp)
				return 3;
			*(nameBuf+(streamSize+basenameSize)) = '\0';	//add termination char
			streamNames[count] = nameBuf;
			count++;

			output = FindNextStreamW(hFindStream, &findStreamData);

			if (output) 
			{	//if continuing, increase streamNames size by 1 for next stream
				//N.B. : count holds current streamSize
				tempSNs = new WCHAR*[count + 1];	//initialize bigger array
				for (int i = 0; i < count; i++) 
					tempSNs[i] = streamNames[i];	//backing up streamNames current values
				delete streamNames;	
				streamNames = tempSNs;				
				//streamNames now bigger and ready for next value
			}
		} while (output);
	}
	/* For Debugging
	if (streamNames)					//names not null
		for (int i = 0; i < count; i++) 
			wcout << "Stream" << i << ":" << *(streamNames + i) << endl;
	*/

	//set Outputs
	streamNamesOut = streamNames;		//output Stream names
	sizeOut = count;					//output number of streams
	FindClose(hFindStream);
	return 0; 
}

void toClipboard(HWND hwnd, const string &s) {
	OpenClipboard(hwnd);
	EmptyClipboard();
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, s.size() + 1);
	if (!hg) {
		CloseClipboard();
		return;
	}
	memcpy(GlobalLock(hg), s.c_str(), s.size() + 1);
	GlobalUnlock(hg);
	SetClipboardData(CF_TEXT, hg);
	CloseClipboard();
	GlobalFree(hg);
}

int HashIncludingADS(string &filename, string &hash) 
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	* Get file name as LPCWSTR
	*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	LPCWSTR lpctName; //= L"C:\\Users\\Mazen\\Documents\\Visual Studio 2015\\Projects\\ADSsum\\Release\\tizi.txt";
	wstring ctemp = s2ws(filename);
	lpctName = ctemp.c_str();
	//cout << lpctName << endl;

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	* Get stream names
	*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	WCHAR** streamNames = NULL;
	int size;
	int errorLevel = ADSs(lpctName, streamNames, size);
	switch (errorLevel) {
		case 1:
			cout << "Can't find file : " << filename << endl;
			return errorLevel;
		case 2:
			cout << "Couldn't copy file path to name buffer" << endl;
			return errorLevel;
		case 3:
			cout << "Couldn't copy stream name to name buffer" << endl;
			return errorLevel;
		case 0:
		default:
			break;
	}
	/*	Debug: see if i can get streamNames that were passed by reference
	for (int i = 0; i < size; i++)
	wcout << "Stream" << i << ":" << *(streamNames + i) << endl;
	*/

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	* Calculate hashes
	*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	string name;
	string* digests = new string[size];
	string digest;
	for (int i = 0; i < size; i++)
	{
		int length = 0;
		wstring tempConv = *(streamNames + i);
		name = ws2s(tempConv);
		char* bytes = readFileBytes(name.c_str(), length);
		digest = GetOSSLSHA256Hash(bytes, length);
		*(digests + i) = digest;
	}
	/* Debugging : stream hashes
	for (int i = 0; i < size; i++)
	cout << "Hash" << i << ": " << endl << *(digests + i) << endl;
	*/

	string overallDigest = *digests; // initialize to first hash

	if (size>1)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		* Get overall hash
		*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		string overall;
		for (int i = 0; i < size; i++)
			overall += *(digests + i);
		char* bytes = (char *)(overall.c_str());
		int overallLength = overall.size();
		overallDigest = GetOSSLSHA256Hash(bytes, overallLength);
		/* Debugging : concatenated hashes
		cout << "CatHashes :" << overall << endl;
		*/

		cout << size << " streams found." << endl;
	}
	cout << "Hash : " << overallDigest << endl;
	hash = overallDigest;
	return 0;
}

vector<string> &split(const string &s, char delim, vector<string> &elems) {
	stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

vector<string> split(const string &s, char delim) {
	vector<string> elems;
	split(s, delim, elems);
	return elems;
}

void errprint(int argc, char* argv[], char* badParam) 
{
	cout << "Bad argument/param : " << badParam << endl;
	cout << "Param(s) Entered : ";
	for (int i = 1; i < argc; i++)
		cout << argv[i] << "|";
	cout << endl << "# of Params Entered : " << (argc - 1) << endl;
	vector<string> splitted = split(argv[0], '\\');
	int sLen = splitted.size();
	string programName = splitted.at(sLen-1);
	cout << "USAGE: \t" << programName << " [PARAMS]" << endl;
	cout << "\t0 params \t: gets input from command line and hashes it" << endl;
	cout << "\t1 param \t: name of file to hash" << endl;
	cout << "\tn params \t: name of each file to hash" << endl;
}

int main(int argc, char *argv[])
{
	string clipOut = "";	//the output to store in clipboard before exit
	if (argc == 2)			//If only filename specified
	{
		string name(argv[1]);
		string hash;
		//wcout << s2ws(name).c_str() << endl ;
		int error = HashIncludingADS(name,hash);
		if (error)
		{
			errprint(argc, argv, argv[1]);
			return error;
		}
		clipOut = hash;
	}
	else if (argc >= 2) {
		string* hashes =  new string[argc - 1];
		string* filenames = new string[argc - 1];
		for (int i = 1; i < argc; i++) {
			string hash;
			string filename = argv[i];
			cout << "File: " << filename << endl;
			int error = HashIncludingADS(filename,hash);
			//to store for clipboard outpute later
			*(hashes + i - 1) = hash;
			*(filenames + i - 1) = filename;		
			if (error)
			{
				errprint(argc, argv, argv[i]);
				return error;
			}
			cout << endl;
		}
		//Setting clipboard string output
		for (int i = 0; i < (argc - 1); i++)
			clipOut += *(filenames + i) + ":\r\n" + *(hashes + i) + "\r\n";
	}
	else if (argc == 1)	//if no param, hash input
	{
		string input;
		cout << "Input : ";
		cin >> input;
		string digest = sha256(input);
		cout << "Hash : " << digest << endl;
		clipOut = digest;
	}
	toClipboard(GetConsoleWindow(), clipOut);
	return 0;
}