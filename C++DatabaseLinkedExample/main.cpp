// HashMe - An example of a File Hash with fixed length records and Heap overflow
// Author - Louis Bradley
// COP 3540 - Introduction to Database STructures

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>
using namespace std;
#define MNRECORDSPERBLOCK 3

struct mdbStudent
{
	string sDelFlag;
	string sID;
	string sLastName;
	string sFirstName;
	string sGPA;
};
struct mdbBlock
{
	int nNumberRecords;
	int nOverflowPointer;
	mdbStudent oStudentRecord[MNRECORDSPERBLOCK];
};

// Function Headers
void Create();
void Dump();
void Insert();
void Delete();
void Print();
long CountRecords();
void CreateOverflowBlock();
mdbStudent ParseRecord(string psString);
int  Find(string psSID);
void Retrieve();
void ReadBuffer(int pnBlock);
void WriteBuffer(int pnBlock);
void ParseBlock(string psBlock);
// Global Variables
const int MNRECORDLENGTH = 48;	// Length of each record 1(Delete Flag) + 3(Student_ID) + 20(Last Name) + 20(First Name) + 4 (GPA)
const int MNBLOCKLENGTH = 10 + MNRECORDSPERBLOCK * MNRECORDLENGTH + 2; // 156
const int MNHASHSIZE = 10;		 // The number of hash blocks that can be held in the primasry area of the file
const string MCSFILENAME = "StudentGPAHash.dat";
fstream mioHashFile;			// The file we will be storing our blocks in
int mnOverFlowRecords = 0;			// number of records in overflow 
mdbBlock mIOBuffer;				// All IO will be from and to this buffer


// Main program that presents a menu of options to the user and calls the various functions
int main()
{
	mioHashFile.open(MCSFILENAME, ios::in | ios::out);
	if (mioHashFile.bad()) // I guess the file wasn't there - Go Create It
	{
		cout << "File Not Found...Creating It" << endl;
		Create();
	}

	char sOption;
	cout << "Enter a Menu Selection - (C)reate, (I)nsert, (R)etrieve, X=Delete, (P)rint, (D)ump, (Q)uit" << endl;
	cin >> sOption;
	while (sOption != 'Q')
	{
		mioHashFile.clear();
		switch (sOption)
		{
		case 'C':
			Create();
			break;
		case 'I':
			Insert();
			break;
		case 'D':
			Dump();
			break;
		case 'X':
			Delete();
			break;
		case 'P':
			Print();
			break;
		case 'R':
			Retrieve();
			break;
		default:
			cout << "Invalid Menu Option Entered \n";
		}

		cout << "Enter a Menu Selection - (C)reate, (I)nsert, (R)etrieve, X=Delete, (P)rint, (D)ump, (Q)uit" << endl;
		cin >> sOption;

	}
	mioHashFile.close();
	return 0;
}
// This function opens the emp file for output which will remove all records already out there
// It then writes a 10 blocks each with an number records, overflow and 3 student record
// For create, we close and open the file for truncate, then close and open it againC
void Create()
{
	mioHashFile.close();
	mioHashFile.open(MCSFILENAME, ios::out | ios::trunc);
	mIOBuffer.nNumberRecords = 0;
	mIOBuffer.nOverflowPointer = -1;
	for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
	{
		mIOBuffer.oStudentRecord[nRecord].sDelFlag = "0";
		mIOBuffer.oStudentRecord[nRecord].sID = "";
		mIOBuffer.oStudentRecord[nRecord].sLastName = "";
		mIOBuffer.oStudentRecord[nRecord].sFirstName = "";
		mIOBuffer.oStudentRecord[nRecord].sGPA = "";
	}
	for (int nBlocks = 0; nBlocks < MNHASHSIZE; nBlocks++)
	{
		WriteBuffer(nBlocks);
	}
	mioHashFile.close();
	cout << "create complete\n";
	mioHashFile.open(MCSFILENAME, ios::in | ios::out);
	return;
}
// This function is used to insert a new record into our student file.
void Insert()
{
	int lastBlock = 0;
	int ioDeleteRecordPointer = -1; // Pointer to the address of the first deleted record
	// Data accepted from CIN 
	mdbStudent ioInput;
	int overFlowTrace = 9;
	int blockNumber = 0;
	int nID;			/* integer value of primary key string sInID */
	int nHashKey;		/* hash value = (key % 10) */
	string sRecord;
	int overflowNumber;



	mnOverFlowRecords = CountRecords();			// Get the number of records in the file.  Needed when we want to add another

	cout << "Please enter Student_ID, Last-Name, First-Name and GPA separated by blanks\n";
	cin >> ioInput.sID >> ioInput.sLastName >> ioInput.sFirstName >> ioInput.sGPA;

	// Check to make sure what was entered wasn't too long
	if (ioInput.sID.length() != 3)
	{
		cout << "ID Must be three characters long.. Please try again" << endl;
		return;
	}
	if (ioInput.sFirstName.length() > 20)
	{
		ioInput.sFirstName = ioInput.sFirstName.substr(0, 20);
	}
	if (ioInput.sLastName.length() > 20)
	{
		ioInput.sLastName = ioInput.sLastName.substr(0, 20);
	}
	if (ioInput.sGPA.length() > 4)
	{
		ioInput.sGPA = ioInput.sGPA.substr(0, 4);
	}

	// Make sure this is a valid ID number
	try
	{
		nID = stoi(ioInput.sID);
	}
	catch (invalid_argument&)
	{
		nID = -1;
	}
	if (nID < 0 || nID > 999)
	{
		cout << "Invalid ID Entered.. Please try again" << endl;
		return;
	}


	// convert primary key to hash by modulas 
	nHashKey = nID  % MNHASHSIZE;
	while (blockNumber != -1)
	{
		overFlowTrace = overFlowTrace + 1;
		// calculate the address in the file for the primary record, set the file pointer
		// there and read the record
		if (blockNumber == 0)
		{
			ReadBuffer(nHashKey);
		}
		else
		{
			overflowNumber = mIOBuffer.nOverflowPointer;
			ReadBuffer(mIOBuffer.nOverflowPointer);
		}

		// Is the block full?
		if (mIOBuffer.nNumberRecords == MNRECORDSPERBLOCK)
		{
			cout << "I'm as stuffed as a turkey at Thanksgiving - No More Room - Hopefully going to overflow!!" << endl;
			//return;
		}


		// Is there a live record with the same SSN?
		for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
		{
			if (mIOBuffer.oStudentRecord[nRecord].sDelFlag.compare("1") == 0 && mIOBuffer.oStudentRecord[nRecord].sID.compare(ioInput.sID) == 0)
			{
				cout << "Student already in file, insert terminated" << endl;
				return;
			}
		}
		//let's find deleted records
		for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
		{
			if (mIOBuffer.oStudentRecord[nRecord].sDelFlag.compare("2") == 0)  // virgin record and block, insert immediately 
			{
				mIOBuffer.oStudentRecord[nRecord] = ioInput;
				mIOBuffer.oStudentRecord[nRecord].sDelFlag = "1";
				mIOBuffer.nNumberRecords = mIOBuffer.nNumberRecords + 1;
				if (blockNumber == 0)
				{
					WriteBuffer(nHashKey);
				}
				else
					WriteBuffer(overflowNumber);
				cout << "Insert Complete" << endl;
				return;
			}
		}

		// Are there any empty slots in this block?
		for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
		{
			if (mIOBuffer.oStudentRecord[nRecord].sDelFlag.compare("0") == 0)  // virgin record and block, insert immediately 
			{
				mIOBuffer.oStudentRecord[nRecord] = ioInput;
				mIOBuffer.oStudentRecord[nRecord].sDelFlag = "1";
				mIOBuffer.nNumberRecords = mIOBuffer.nNumberRecords + 1;
				if (blockNumber == 0)
				{
					WriteBuffer(nHashKey);
				}
				else
					WriteBuffer(overflowNumber);
				cout << "Insert Complete" << endl;
				return;
			}
		}
		if (blockNumber == 0)
		{
			lastBlock = nHashKey;
		}
		else
			lastBlock = blockNumber;

		blockNumber = mIOBuffer.nOverflowPointer;

	}
	//	overFlowTrace;
	//mIOBuffer.nOverflowPointer = overFlowTrace;
	CreateOverflowBlock();
	ReadBuffer(overFlowTrace);
	for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
	{
		if (mIOBuffer.oStudentRecord[nRecord].sDelFlag.compare("0") == 0)  // virgin record and block, insert immediately 
		{
			mIOBuffer.oStudentRecord[nRecord] = ioInput;
			mIOBuffer.oStudentRecord[nRecord].sDelFlag = "1";
			mIOBuffer.nNumberRecords = mIOBuffer.nNumberRecords + 1;
			WriteBuffer(overFlowTrace);
			cout << "Insert Complete" << endl;
			ReadBuffer(lastBlock);
			mIOBuffer.nOverflowPointer = overFlowTrace;
			WriteBuffer(lastBlock);
			return;
		}
	}
	// Not sure how we got here since I can't delete records yet, so give an error message and get out
	cout << "really strange error in Insert.   How the heck did I get here?" << endl;
	return;

}
// This function simply uses the DOS Type command to dump the contents of the file
void Dump()
{
	string sCommand = "type " + MCSFILENAME;
	system(sCommand.c_str());
	return;
}

//This function prints out all the records in the employee file including 
//primary and overflow records.
void Print()
{
	int nNumberRecords = 0;
	mioHashFile.clear();
	mioHashFile.seekg(0);
	/* sequentially print all active records */
	string sBlock;
	getline(mioHashFile, sBlock);
	while (!mioHashFile.eof())
	{
		ParseBlock(sBlock);
		for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
		{
			if (mIOBuffer.oStudentRecord[nRecord].sDelFlag.compare("1") == 0)
			{
				cout << "Student ID = " << mIOBuffer.oStudentRecord[nRecord].sID << " Name = " << mIOBuffer.oStudentRecord[nRecord].sFirstName;
				cout << " " << mIOBuffer.oStudentRecord[nRecord].sLastName << " GPA = " << mIOBuffer.oStudentRecord[nRecord].sGPA << endl;
				nNumberRecords = nNumberRecords + 1;
			}
		}
		getline(mioHashFile, sBlock);
	}
	cout << "Print Complete...Number of Records = " << nNumberRecords << endl << endl;
	return;
}

// This function gets the beginning and end of the file to calculate the number of records
long CountRecords()
{
	streampos nBegin, nEnd;
	nBegin = mioHashFile.tellg();
	mioHashFile.seekg(0, ios::end);
	nEnd = mioHashFile.tellg();
	return ((nEnd - nBegin) / MNBLOCKLENGTH);
}

// The function accepts a block number and then writes the current block in memory back out
// to the file.  It uses the String Stream library to concatenate the information.
void WriteBuffer(int pnBlock)
{
	mioHashFile.seekp(pnBlock * MNBLOCKLENGTH);
	ostringstream sBuffer;
	sBuffer.trunc;
	mioHashFile << setw(5) << mIOBuffer.nNumberRecords << setw(5) << mIOBuffer.nOverflowPointer;
	for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
	{
		mioHashFile << setw(1) << mIOBuffer.oStudentRecord[nRecord].sDelFlag << setw(3) << mIOBuffer.oStudentRecord[nRecord].sID << setw(20) << left << mIOBuffer.oStudentRecord[nRecord].sLastName;
		mioHashFile << setw(20) << left << mIOBuffer.oStudentRecord[nRecord].sFirstName << setw(4) << right << mIOBuffer.oStudentRecord[nRecord].sGPA;
	}
	mioHashFile << endl;

}
// This function sets the Get seek to a specific block and reads the block into memory
void ReadBuffer(int pnBlock)
{
	mioHashFile.seekg(pnBlock * MNBLOCKLENGTH);
	string sBuffer;
	getline(mioHashFile, sBuffer);
	ParseBlock(sBuffer);
}
// This function takes a string from the file and converts it to a Block structure in memory
void ParseBlock(string psBlock)
{
	mIOBuffer.nNumberRecords = stoi(psBlock.substr(0, 5));
	mIOBuffer.nOverflowPointer = stoi(psBlock.substr(5, 5));
	for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
	{
		mIOBuffer.oStudentRecord[nRecord] = ParseRecord(psBlock.substr(10 + nRecord * MNRECORDLENGTH, MNRECORDLENGTH));
	}
}
// Function that parses a record and returns the fields in a structure
mdbStudent ParseRecord(string psRecord)
{
	mdbStudent dbHoldRecord;
	dbHoldRecord.sDelFlag = psRecord.substr(0, 1);
	dbHoldRecord.sID = psRecord.substr(1, 3);
	dbHoldRecord.sLastName = psRecord.substr(4, 20);
	dbHoldRecord.sFirstName = psRecord.substr(24, 20);
	dbHoldRecord.sGPA = psRecord.substr(44, 4);
	return dbHoldRecord;
}
//****************************************************************************************************
//Functions that don't work yet
long  find(string psSID)
{
	return 0;
}


void Retrieve()
{
	//int numberOfPrintedRecords = 0;
	string fieldName;
	string data;
	cout << "Enter id, last, first, OR gpa, followed by the data you wish to find" << endl;
	cin >> fieldName >> data;
	if (fieldName != "id" && fieldName != "last" && fieldName != "first" && fieldName != "gpa")
	{
		cout << "error with field name input" << endl;
		return;
	}
	//cout << data;
	if (fieldName == "id" && data.length() > 3)
	{
		cout << "error with ID count" << endl;
		return;
	}

	if (data.length() > 20)
	{
		cout << "error with data length" << endl;
		return;
	}

	int nNumberRecords = 0;
	mioHashFile.clear();
	mioHashFile.seekg(0);
	/* sequentially print all active records */
	string sBlock;
	getline(mioHashFile, sBlock);
	while (!mioHashFile.eof())
	{
		ParseBlock(sBlock);
		for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
		{
			if (fieldName == "last")
			{
				//cout << mIOBuffer.oStudentRecord[nRecord].sLastName << "+" << endl;
				//cout << data << "+" << endl;
				data.resize(20, ' ');
				if (mIOBuffer.oStudentRecord[nRecord].sLastName.compare(data) == 0)
				{
					cout << "Student ID = " << mIOBuffer.oStudentRecord[nRecord].sID << " Name = " << mIOBuffer.oStudentRecord[nRecord].sFirstName;
					cout << " " << mIOBuffer.oStudentRecord[nRecord].sLastName << " GPA = " << mIOBuffer.oStudentRecord[nRecord].sGPA << endl;
					nNumberRecords = nNumberRecords + 1;
				}
			}
			if (fieldName == "id")
			{
				if (data == mIOBuffer.oStudentRecord[nRecord].sID)
				{
					cout << "Student ID = " << mIOBuffer.oStudentRecord[nRecord].sID << " Name = " << mIOBuffer.oStudentRecord[nRecord].sFirstName;
					cout << " " << mIOBuffer.oStudentRecord[nRecord].sLastName << " GPA = " << mIOBuffer.oStudentRecord[nRecord].sGPA << endl;
					nNumberRecords = nNumberRecords + 1;
				}
			}
			if (fieldName == "first")
			{
				data.resize(20, ' ');
				if (data == mIOBuffer.oStudentRecord[nRecord].sFirstName)
				{
					cout << "Student ID = " << mIOBuffer.oStudentRecord[nRecord].sID << " Name = " << mIOBuffer.oStudentRecord[nRecord].sFirstName;
					cout << " " << mIOBuffer.oStudentRecord[nRecord].sLastName << " GPA = " << mIOBuffer.oStudentRecord[nRecord].sGPA << endl;
					nNumberRecords = nNumberRecords + 1;
				}
			}
			if (fieldName == "gpa")
			{
				if (data == mIOBuffer.oStudentRecord[nRecord].sGPA)
				{
					cout << "Student ID = " << mIOBuffer.oStudentRecord[nRecord].sID << " Name = " << mIOBuffer.oStudentRecord[nRecord].sFirstName;
					cout << " " << mIOBuffer.oStudentRecord[nRecord].sLastName << " GPA = " << mIOBuffer.oStudentRecord[nRecord].sGPA << endl;
					nNumberRecords = nNumberRecords + 1;
				}
			}


		}
		getline(mioHashFile, sBlock);
	}
	cout << "Retrive Complete...Number of Records = " << nNumberRecords << endl << endl;
	return;
	//cout << "Can't Retreive Yet.  What good am I?" << endl;
	//return;

}


void Delete()
{
	int nID;
	int hashKey;
	string input;
	cout << "Enter an ID you wish to delete" << endl;
	cin >> input;
	if (input.length() > 3 || input.length() < 3)
	{
		cout << "error with ID length" << endl;
	}
	// Make sure this is a valid ID number
	try
	{
		nID = stoi(input);
	}
	catch (invalid_argument&)
	{
		nID = -1;
	}
	if (nID < 0 || nID > 999)
	{
		cout << "Invalid ID Entered.. Please try again" << endl;
		return;
	}

	hashKey = nID  % MNHASHSIZE;



	int nNumberRecords = 0;
	mioHashFile.clear();
	mioHashFile.seekg(0);
	/* sequentially print all active records */
	string sBlock;
	int curBlock = -1;
	getline(mioHashFile, sBlock);
	while (!mioHashFile.eof())
	{
		curBlock++;
		ParseBlock(sBlock);
		for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
		{
			if (mIOBuffer.oStudentRecord[nRecord].sDelFlag.compare("1") == 0)
			{
				if (mIOBuffer.oStudentRecord[nRecord].sDelFlag.compare("1") == 0 && mIOBuffer.oStudentRecord[nRecord].sID.compare(input) == 0)
				{
					mIOBuffer.oStudentRecord[nRecord].sDelFlag = "2";
					mIOBuffer.nNumberRecords = mIOBuffer.nNumberRecords - 1;
					WriteBuffer(curBlock);
					cout << "delete complete" << endl;
					//nNumberRecords = nNumberRecords - 1;
					return;
				}

			}
		}
		//lastBlock = mIOBuffer.nOverflowPointer;
		getline(mioHashFile, sBlock);
	}
	//cout << "Print Complete...Number of Records = " << nNumberRecords << endl << endl;
	return;
	//cout << "Can't Delete Yet.  So Sorry. " << endl;
	
}

void CreateOverflowBlock(){
	mioHashFile.seekg(0, ios::end);
	mIOBuffer.nNumberRecords = 0;
	mIOBuffer.nOverflowPointer = -1;
	for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
	{
		mIOBuffer.oStudentRecord[nRecord].sDelFlag = "0";
		mIOBuffer.oStudentRecord[nRecord].sID = "";
		mIOBuffer.oStudentRecord[nRecord].sLastName = "";
		mIOBuffer.oStudentRecord[nRecord].sFirstName = "";
		mIOBuffer.oStudentRecord[nRecord].sGPA = "";
	}
	//mioHashFile.seekp(pnBlock * MNBLOCKLENGTH);
	ostringstream sBuffer;
	sBuffer.trunc;
	mioHashFile << setw(5) << mIOBuffer.nNumberRecords << setw(5) << mIOBuffer.nOverflowPointer;
	for (int nRecord = 0; nRecord < MNRECORDSPERBLOCK; nRecord++)
	{
		mioHashFile << setw(1) << mIOBuffer.oStudentRecord[nRecord].sDelFlag << setw(3) << mIOBuffer.oStudentRecord[nRecord].sID << setw(20) << left << mIOBuffer.oStudentRecord[nRecord].sLastName;
		mioHashFile << setw(20) << left << mIOBuffer.oStudentRecord[nRecord].sFirstName << setw(4) << right << mIOBuffer.oStudentRecord[nRecord].sGPA;
	}
	mioHashFile << endl;



}