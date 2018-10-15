#include <cstdlib>
#include <string>

//이 형식은 직접 메모리를 참조하지 않는 스트림 버퍼 C버전으로 만든 것이다.
//함수를 호출하면 ptr에 siz만큼의 메모리가 복사된다.
struct streamobj;
typedef int (*stream)(streamobj* s,int siz,void* ptr);
struct streamobj
{
	bool end;
	int id;
	stream str;
};


class fat32_folder
{
public:
	char name[11];
	char attribut;
	char NT,Ctt;
	short CT;
	short CreateDate;
	short last_Acc_Date;
	short clusterh;
	short writeTime;
	short WriteDate;
	short clusterl;
	int fileSize;
	inline int get_cluster()
	{
		return ((int)clusterh<<16)|clusterl;
	}
	inline void set_cluster(int cluster)
	{
		clusterl=(short)cluster;
		clusterh=(short)(cluster>>16);
	}
};
void init_fat32();
void fat32_floppy_buffclear();
void fat32_add(int fld1,fat32_folder* inf,streamobj in_str);
void insert_block(int fld1,char d[0x20]);
//빈 클러스터를 찾는다
int findempty(int k);
//새 폴더를 만든다.
void fat32_new_folder(int fld1,fat32_folder* fd);

char* getname11(char ret[11],const std::string name);
void native_rm(std::string filedirectory);
void native_cp(std::string a,std::string b);
void native_mv(std::string a,std::string b);
void native_add(std::string a,fat32_folder* fd,streamobj str);