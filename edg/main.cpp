#include <stdio.h>
#include <Windows.h>
#include <string>
#include <Windows.h>
#include "fat32.h"
using namespace std;
/////////////////////////////////////data
struct fat_intstruct {
	char BPB_OEM[8] = { 0 };
	short BPB_BYTE_PER_SECTOR;
	char SECTOR_PER_CLUSTER;
	short BPB_RESERVED_SECTOR;
	char FAT_NUM;
	short BPB_ROOT_ENTRY_NUMBER;
	short BPB_FAT16_SECTORS;
	char BPB_MEDIA_TYPE;
	short BPB_FAT16_SIZE;
	short BPB_SECTOR_PER_TRACK;
	short BPB_HEADS;
	int BPB_HIDDEN_SECTOR;
	int BPB_FAT32_SECTORS;
	int BPB_FAT32_SIZE;
	short BPB_EXTENSION_FLAGS;
	short BPB_FILESYSTEM_VER;
	int BPB_ROOT_ENTRY_CLUSTER;
	short BPB_FILESYSTEM_INFO;
	short BPB_BACKUP_BOOT_SECTOR;
	int BPB_RESERVED[3];
	char BS_DRIVE;
	char BS_RESERVED1;
	char BS_BOOT_SIGNATURE;
	int BS_VOLUME_ID;
	char BS_VOLUME_LABEL[11] = { 0 };
	char BS_FILESYSTEM_TYPE[8] = { 0 };
} fs;
char empty;
char buf[512];
FILE* p;
FILE* o;
char* FLOPPY_BUFFER=buf;
/////////////////////////////////////////////////
void floppy_read(int lin)
{
	fseek(o,lin*512,0);
	fread(FLOPPY_BUFFER,512,1,o);
}
void floppy_write(int lin)
{
	fseek(o,lin*512,0);
	fwrite(FLOPPY_BUFFER,512,1,o);
}
void loadfolder(int,int,string,bool);
#define BPB_string(typ,BPB,BPBSTR,SIZE) fread(&(fs.##BPB),1,SIZE,p);\
	printf("%s",BPBSTR);\
	printf(typ,(fs.##BPB))
#define BPB_PRINT(BPB,BPBSTR) BPB_string("%d\n",BPB,BPBSTR,sizeof(fs.##BPB))
#define error1(ERN,STR) if(ERN)\
{\
	fputs(STR,stderr);\
	exit(1);\
}
#define warning1(ERN,STR) if(ERN)\
{\
	fputs(STR,stderr);\
}
//#include "inline.h"

int main(int argc, char *argv[])
{
	error1(argc == 1, "usage:\n\
			  edg -bs :[bootsector.bif] -rd:[rootdirectory] -out:[fat.img]\
			  ");
	string arg;
	{/*for*/
		int i = 1;
	L1:

		arg += argv[i];
		i++;
		if (i < argc)
		{
			arg += ' ';
			goto L1;
		}
	}
	bool bs = false, rd = false, ot = false, oi = true;
	string bootname;
	string rootdir;
	string outfile;
	int i = 0;
	try
	{
		for (; arg[i] == ' '; i++);

		for (; i < arg.length();)
		{
			if (arg[i] == '-')
			{
				i++;
				for (; arg[i] == ' '; i++);
				if (arg.substr(i, 2) == "bs")
				{
					i += 2;
					error1(bs, "redefined bs");
					bs = true;
					for (; arg[i] == ' '; i++);
					if (arg[i] == ':')
					{
						i++;
						for (; arg[i] == ' '; i++);
					}
					for (; arg[i] == ' '; i++);
					int j = i;
					for (; i < arg.length() && arg[i] != ' '; i++);
					bootname = arg.substr(j, i - j);

				}
				else if (arg.substr(i, 3) == "out")
				{
					i += 3;
					error1(ot, "redefined bs");
					ot = true;
					for (; arg[i] == ' '; i++);
					if (arg[i] == ':')
					{
						i++;
						for (; arg[i] == ' '; i++);
					}
					for (; arg[i] == ' '; i++);
					int j = i;
					for (; i < arg.length() && arg[i] != ' '; i++);
					outfile = arg.substr(j, i - j);

				}
				else if (arg.substr(i, 2) == "oq")
				{
					i += 2;
					oi = true;
					for (; arg[i] == ' '; i++);
				}
				else if (arg.substr(i, 2) == "rd")
				{
					i += 2;
					error1(rd, "redefined bs");
					rd = true;
					for (; arg[i] == ' '; i++);
					if (arg[i] == ':')
					{
						i++;
						for (; arg[i] == ' '; i++);
					}
					for (; arg[i] == ' '; i++);
					int j = i;
					for (; i < arg.length() && arg[i] != ' '; i++);
					rootdir = arg.substr(j, i - j);

				}
				else
					error1(true, "arguments err");
			}
			else
			{
				for (; arg[i] == ' '; i++);
				int j = i;
				for (; arg[i] != ' '; i++);
				string tmp = arg.substr(j, i - j);
				j = tmp.find_last_of('.');
				if (j == -1)
				{
					error1(rd, "arguments err");
					rd = true;
					rootdir = tmp;
				}
				else if (tmp.substr(j + 1) == "bif")
				{
					error1(bs, "arguments err");
					bs = true;
					bootname = tmp;
				}
				else if (tmp.substr(j + 1) == "img")
				{
					error1(ot, "arguments err");
					ot = true;
					outfile = tmp;

				}
				else
				{
					fputs("arguments err", stderr);
					return 1;
				}

			}
			for (; arg[i] == ' '; i++);

		}
	}
	catch (out_of_range e)
	{
		fputs("arguments err", stderr);
		return 1;
	}
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////



	fopen_s(&p, bootname.c_str(), "rb");/*읽을 수 있는 권한으로 파일을 연다.*/
	if (!p)
		return -1;
	fopen_s(&o, outfile.length() != 0 ? outfile.c_str() : "out.img", "w+b");
	if (!o)
		return -1;

	fseek(p, 3, 0);
	BPB_string("%s\n", BPB_OEM, "BPB_OEM: ", 8);
	BPB_PRINT(BPB_BYTE_PER_SECTOR, "BPB_BYTE_PER_SECTOR: ");
	BPB_PRINT(SECTOR_PER_CLUSTER, "SECTOR_PER_CLUSTER: ");
	BPB_PRINT(BPB_RESERVED_SECTOR, "BPB_RESERVED_SECTOR: ");
	BPB_PRINT(FAT_NUM, "FAT_NUM: ");
	BPB_PRINT(BPB_ROOT_ENTRY_NUMBER, "BPB_ROOT_ENTRY_NUMBER: ");
	BPB_PRINT(BPB_FAT16_SECTORS, "BPB_FAT16_SECTORS: ");
	BPB_PRINT(BPB_MEDIA_TYPE, "BPB_MEDIA_TYPE: ");
	BPB_PRINT(BPB_FAT16_SIZE, "BPB_FAT16_SIZE: ");
	BPB_PRINT(BPB_SECTOR_PER_TRACK, "BPB_SECTOR_PER_TRACK: ");
	BPB_PRINT(BPB_HEADS, "BPB_HEADS: ");
	BPB_PRINT(BPB_HIDDEN_SECTOR, "BPB_HIDDEN_SECTOR: ");
	BPB_PRINT(BPB_FAT32_SECTORS, "BPB_FAT32_SECTORS: ");
	BPB_PRINT(BPB_FAT32_SIZE, "BPB_FAT32_SIZE: ");
	BPB_PRINT(BPB_EXTENSION_FLAGS, "BPB_EXTENSION_FLAGS: ");
	BPB_PRINT(BPB_FILESYSTEM_VER, "BPB_FILESYSTEM_VER: ");
	BPB_PRINT(BPB_ROOT_ENTRY_CLUSTER, "BPB_ROOT_ENTRY_CLUSTER: ");
	BPB_PRINT(BPB_FILESYSTEM_INFO, "BPB_FILESYSTEM_INFO: ");
	BPB_PRINT(BPB_BACKUP_BOOT_SECTOR, "BPB_BACKUP_BOOT_SECTOR: ");
	BPB_PRINT(BPB_RESERVED[0], "BPB_RESERVED1: ");
	BPB_PRINT(BPB_RESERVED[1], "BPB_RESERVED2: ");
	BPB_PRINT(BPB_RESERVED[2], "BPB_RESERVED3: ");
	BPB_PRINT(BS_DRIVE, "BS_DRIVE: ");
	BPB_PRINT(BS_RESERVED1, "BS_RESERVED1: ");
	BPB_PRINT(BS_BOOT_SIGNATURE, "BS_BOOT_SIGNATURE: ");
	BPB_PRINT(BS_VOLUME_ID, "BS_VOLUME_ID: ");
	BPB_string("%s\n", BS_VOLUME_LABEL, "BS_VOLUME_LABEL: ", 11);
	BPB_string("%s\n", BS_FILESYSTEM_TYPE, "BS_FILESYSTEM_TYPE: ", 8);
	fseek(p, 0, SEEK_END);
	for (int i = ftell(p); ;) {
		if (i >= 512) {
			fread(buf, 512, 1, p);
			fwrite(buf, 512, 1, o);
			i -= 512;
		}
		else if (i == 0) {
			break;
		}
		else {
			memset(buf, 0, 512);
			fread(buf, i, 1, p);
			fwrite(buf, 512, 1, o);
			break;
		}
	}
	memset(buf,0,512);
	
	for(int i=ftell(p)/512;i<fs.BPB_FAT32_SECTORS*2;i++)
	{
		fwrite(buf,512,1,o);
	}
	init_fat32();
	loadfolder(fs.BPB_ROOT_ENTRY_CLUSTER,2,rootdir,true);	
}
int fstreamsub(streamobj* thiss,int,void* p)
{
	static FILE* id1=0;
	if(id1!=(FILE*)(thiss->id))
	{
		id1=(FILE*)(thiss->id);
	}
	if(feof(id1))
		return 0;
	fread(p,0x200,1,id1);
	if(feof(id1))
		thiss->end=true;
	return 0x200;
}
#include <vector>
void loadfolder(int foldercluster,int upper,string dirname,bool isroot)
{
	char tbuf[512];
	vector<pair<int,string>> folderdata;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFile((dirname+"\\*").c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		printf ("reading driectory failed (%d)\n", GetLastError());
		exit(1);
	}
	while(FindNextFile(hFind,&FindFileData))
	{
		if(FindFileData.nFileSizeHigh)
		{
			printf ("file %s is too big\n", FindFileData.cFileName);
		}
		//int isc=ftell(o);
		int cnt=0;
		
		//기록 위치를 찾았으면 이제 파일의 속성들을 기록한다.
		fat32_folder fd;
		fd.attribut=FindFileData.dwFileAttributes&0x3F;
		*(short*)&fd.NT=0xAB18;
		fd.CT=FindFileData.ftCreationTime.dwHighDateTime;
		fd.CreateDate=FindFileData.ftLastAccessTime.dwHighDateTime>>16;
		fd.last_Acc_Date=FindFileData.ftLastAccessTime.dwHighDateTime>>16;
		fd.writeTime=FindFileData.ftCreationTime.dwHighDateTime;
		fd.WriteDate=FindFileData.ftCreationTime.dwHighDateTime;
		fd.fileSize=FindFileData.nFileSizeLow;
		////////////////////////////////////////////////////
		/////////////////////파일을 기록한다.
		if(!strcmp(FindFileData.cFileName,"."))
		{
			if(isroot)
				continue;
			memcpy(fd.name,".          ",11);
			fd.set_cluster(foldercluster);
			insert_block(foldercluster,(char*)&fd);
		}
		else if(!strcmp(FindFileData.cFileName,".."))
		{
			if(isroot)
				continue;
			memcpy(fd.name,"..         ",11);
			fd.set_cluster(upper);
			insert_block(foldercluster,(char*)&fd);
		}
		else if(FindFileData.dwFileAttributes&0x10)
		{
			getname11(fd.name,FindFileData.cFileName);
			fat32_new_folder(foldercluster,&fd);
			folderdata.push_back(pair<int,string>(fd.get_cluster(),FindFileData.cFileName));
		}
		else
		{
			streamobj tem={0,0,fstreamsub};
			fopen_s((FILE**)&tem.id,(dirname+'\\'+FindFileData.cFileName).c_str(),"rb");
			getname11(fd.name,FindFileData.cFileName);
			fat32_add(foldercluster,&fd,tem);
			fclose((FILE*)tem.id);
		}
	}
	for(auto i=folderdata.begin();i!=folderdata.end();i++)
	{
		loadfolder(i->first,foldercluster,dirname+'\\'+i->second,false);
	}
	FindClose(hFind);
	fat32_floppy_buffclear();
}
