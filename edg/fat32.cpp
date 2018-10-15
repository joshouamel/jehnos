#include "fat32_floppysetting.h"
#include "fat32.h"
int fatsec=0;
bool com=false;

void fat32_floppy_buffclear()
{
	if(com)
		{
			floppy_write((BPB_HIDDEN_SECTOR+BPB_RESERVED_SECTOR)* SECTOR_PER_CLUSTER+fatsec);
			com=false;
		}
}
//파일을 플로피에서 읽고 쓰는 역할을 한다. com으로 수정된 정보를 다시 플로피로 쓰게 설정한다.
void readbasesect(int fs)
{
	if(fs!=fatsec)
	{
		fat32_floppy_buffclear();
		fatsec=fs;
		floppy_read((BPB_HIDDEN_SECTOR+BPB_RESERVED_SECTOR)* SECTOR_PER_CLUSTER+fatsec);
	}
}
//현재 위치를 강제 변경한다.
void set_fatsec(int cluster,int page)
{
	fatsec=(BPB_FAT32_SIZE*FAT_NUM +cluster-BPB_ROOT_ENTRY_CLUSTER)*SECTOR_PER_CLUSTER+page;
}
//현재 위치를 변경한다.
void fixed_fatsec(int cluster,int page)
{
	fat32_floppy_buffclear();
	fatsec=(BPB_FAT32_SIZE*FAT_NUM +cluster-BPB_ROOT_ENTRY_CLUSTER)*SECTOR_PER_CLUSTER+page;
}
//파일을 fat32 시스템으로 클러스터를 읽는다.
void readcluster(int cluster,int page)
{
	int posx=(BPB_FAT32_SIZE*FAT_NUM +cluster-BPB_ROOT_ENTRY_CLUSTER)* SECTOR_PER_CLUSTER;
	readbasesect(posx+page);
}
//파일을 fat32시스템으로 다음 클러스터를 얻어온다.
int getcluster(int cluster)
{
	int posx,posy;
	_asm
	{
		mov eax,[cluster]
		cdq
		mov ecx,80h
		div ecx
		mov [posx],eax
		mov [posy],edx
	}
	readbasesect(posx);
	return *(((int*)FLOPPY_BUFFER)+cluster%128);
}
//파일을 fat32시스템으로 다음 클러스터를 설정한다.
void setcluster(int cluster,int val)
{
	if(cluster==0x0fffffff)
		return;
	int posx,posy;
	_asm
	{
		mov eax,[cluster]
		cdq
		mov ecx,80h
		div ecx
		mov [posx],eax
		mov [posy],edx
	}
	readbasesect(posx);
	if(*(((int*)FLOPPY_BUFFER)+posy)!=val)
	{
		com=true;
		*(((int*)FLOPPY_BUFFER)+posy)=val;
	}
}
inline void cluster_clean(int c)
{
	for(int i=0;i<SECTOR_PER_CLUSTER;i++)
	{
		readcluster(c,i);
		memset(FLOPPY_BUFFER,0,0x200);
		com=1;
	}
}
//zero인지 아닌지 판단한다. 다 0이면 0, 아니면 1 이상.
int memifzero(void* p,int sizew)
{
	_asm
	{
		mov edi,[p]
		xor eax,eax
		mov ecx,[sizew]
		repe scas [edi]
		mov eax,ecx
	}
}
//빈 클러스터를 찾는다
int findempty(int k)
{
	if(k==0x0fffffff)
		return k;
	while(true)
	{
		int p=getcluster(++k);
		if(!p)
			return k;
	}
}

void init_fat32()
{
	char buf[0x20]={0};
	setcluster(BPB_ROOT_ENTRY_CLUSTER,0x0fffffff);
	*(int*)buf=0xBA20F5BB;//루트디렉토리 초기화
	*(int*)(buf+4)=0xBCB7FD20;//
	*(int*)(buf+8)=0x08202020;//
	insert_block(BPB_ROOT_ENTRY_CLUSTER,buf);

	setcluster(0,(unsigned int)0x0FFFFF00+(unsigned char)BPB_MEDIA_TYPE);
	setcluster(1,0xFFFFFFFF);
}
/*
//해당 파일의 연결을 삭제하기.
void fat32_rm_sub_decon_hide(int cluster)
			{
	int next=cluster;
	while(next!=0x0fffffff);
	{
		setcluster(next,0);
		next=getcluster(next);
	}
}
//해당 폴더 안의 파일의 연결을 삭제하기.재귀함수임.
void fat32_rm_sub_folddecon_hide(int cluster)
{
	int index=2;
	int page=0;
	while(true)
	{
	if(cluster==0x0fffffff) break;
		readcluster(cluster,page);
	for(;index++<0x10;)
	{
		int pc=((int)*(unsigned short*)(FLOPPY_BUFFER+index*0x20+0x14))<<16+*(unsigned short*)(FLOPPY_BUFFER+index*0x20+0x1a);
		if(pc==0)break;
		if(*(char*)(FLOPPY_BUFFER+index*0x20+0xc)&=0x80) fat32_rm_sub_folddecon_hide(pc);
		else fat32_rm_sub_decon_hide(pc);
	}
	index=0;
	if(++page==SECTOR_PER_CLUSTER)
	{
		int t=getcluster(cluster);
		setcluster(cluster,0);
		cluster=t;
		page=0;
	}
	}
}
//해당 index의 폴더및 파일을 list에서만 제거한다.
void fat32_rm_sub_hide(int cluster,int page,int index)
{
	readcluster(cluster,page);

	//해당 페이지의 list를 변경한다. 즉 폴더 혹은 파일을 삭제한다.
	memmove(FLOPPY_BUFFER+index*0x20,FLOPPY_BUFFER+index*0x20+0x20,(0x10-1-index)*0x20);

	//그 다음 페이지부터 list를 변경한다.
	int nc,np,nnc=cluster;
	char tbuf[0x20];
	while(true)
	{
		nc=cluster,np=page;
		page^=1;
		cluster=nnc;
		if(page)nnc=getcluster(nnc);
		readcluster(cluster,page);com=true;
		memcpy(tbuf,FLOPPY_BUFFER,0x20);
		memmove(FLOPPY_BUFFER,FLOPPY_BUFFER+0x20,(0x10-1)*0x20);
		if(nnc==0x0fffffff)
			memset(FLOPPY_BUFFER+(0x10-1)*0x20,0,0x20);
		
		if(np&&!memifzero(FLOPPY_BUFFER,0x20))
			page=2;
		readcluster(nc,np);com=true;
		memcpy(FLOPPY_BUFFER+(0x10-1)*0x20,tbuf,0x20);
		if(page==2)
		{
			setcluster(nc,0x0fffffff);
			setcluster(cluster,0);
			return;
		}
		if(nnc==0x0fffffff)return;
	}
}
//폴더안의 index를 삭제한다. 사용 불필요.
void fat32_rm_index(int fld1,int index)
{
	int cluster=fld1;
	int page=0;
	for(;index>=0x10*SECTOR_PER_CLUSTER;index-=0x10*SECTOR_PER_CLUSTER)
	{
		cluster=getcluster(cluster);
	}
	for(;index>=0x10;index-=0x10)page++;
	readcluster(cluster,page);
	int hos=((int)*(unsigned short*)(FLOPPY_BUFFER+index*0x10+0x14))<<16+*(unsigned short*)(FLOPPY_BUFFER+index*0x10+0x1a);
	if(hos==0);
		if(*(char*)(FLOPPY_BUFFER+index*0x20+0xc)&=0x80) fat32_rm_sub_folddecon_hide(hos);
		else fat32_rm_sub_decon_hide(hos);
	fat32_rm_sub_hide(cluster,page,index);
}*/
//인덱스를 찾는다 _in_ int fld,char[11] name,label brk _out_ int nxt,int page,int index
inline void fat32_find_index(int fld, char name[11],int& clt,int& page,int& index)
{
	int cntc=0;
	while(true)
	{
		for(page=0;page<SECTOR_PER_CLUSTER;page++){
			readcluster(clt,page);
			for(index=0;index<0x10;index++)
			{
				if(!memcmp(FLOPPY_BUFFER+index*0x20,name,11))
				{
					return;
				}else
				if(!memifzero(FLOPPY_BUFFER+index*0x20,0x20))
				{
					return;
				}
			}
		}
		clt=getcluster(fld);
		cntc++;
		if(clt==0x0fffffff)
			return;
	}
}
/*

//파일및 폴더를 지운다. 수정필요//길이에 대해서
void fat32_rm(int fld1,char d[11])
	//디렉토리를 지울 경우 폴더 안의 파일들을 지운다.
{
	int nxt=fld1;
	int cntc=0;
	int i;
	int j;
	fat32_find_index(fld1,d,nxt,i,j);
	int hos=((fat32_folder*)(FLOPPY_BUFFER+j*0x20))->get_cluster();
	//폴더 혹은 파일의 연결을 모두 끊어주고
	if(*(char*)(FLOPPY_BUFFER+j*0x20+0xc)&=0x80) fat32_rm_sub_folddecon_hide(hos);
	else fat32_rm_sub_decon_hide(hos);
	//list에서 제거
	fat32_rm_sub_hide(nxt,i,j);
}
*/
//폴더의 list에 정보를 삽입
void insert_block(int fld1,char d[0x20])
{
	int c;
	do{
		c=fld1;
		fld1=getcluster(c);
	}while(fld1!=0x0fffffff);
	int p=0;
#define ploop(x) \
	readcluster(c,x);\
	for(int k=0;k<0x200;k+=0x20)\
	{\
		if(memifzero(FLOPPY_BUFFER+k,0x20))continue;\
		else\
		{\
			memcpy(FLOPPY_BUFFER+k,d,0x20);\
			com=1;\
			return;\
		}\
	}
	for(int i=0;i<SECTOR_PER_CLUSTER;i++)
	{ploop(i)}
#undef ploop
	fld1=findempty(2);
	setcluster(c,fld1);
	setcluster(fld1,0x0fffffff);
	readcluster(fld1,0);
	memcpy(FLOPPY_BUFFER,d,0x20);
	memset(FLOPPY_BUFFER+0x20,0,0x1d0);
	com=1;
	for(int i=1;i<SECTOR_PER_CLUSTER;i++)
	{
		readcluster(fld1,i);
	memset(FLOPPY_BUFFER,0,0x200);
	com=1;
	}
}/*
//파일을 옮긴다.
void fat32_mv(int fld1,int fld2,char d[11],char s[11])
{
	int nxt=fld2;
	int i;
	int j;
	fat32_find_index(fld2,d,nxt,i,j);
	char tbuf[0x20];
	readcluster(nxt,i);
	memcpy(tbuf,FLOPPY_BUFFER+j*0x20,0x20);
	fat32_rm_sub_hide(nxt,i,j);
	memcpy(tbuf,s,11);
	insert_block(fld1,tbuf);
}*/

//포인터가 끊긴 추가된 파일을 반환한다. 반환된 값을 저장해줘야 한다.
int fat32_add_sub_hide(streamobj in_str)
{
	int crr=findempty(2);
	int ret=crr;
	while(true)
	{
		//crr에다 씀
		for(int i=0;i<SECTOR_PER_CLUSTER;i++)
		{
			fixed_fatsec(crr,i);
		in_str.str(&in_str,0x200,FLOPPY_BUFFER);
		com=1;
		if(in_str.end)
			goto ob;
		}
		int cr=findempty(crr);
		setcluster(crr,cr);
		crr=cr;
		}//항상 0보다 크므로 씀
	ob:
	setcluster(crr,0x0fffffff);
	return ret;
}
//파일을 추가한다. inf는 파일의 정보를 나타낸다. 클러스터만 비워주면 된다. streamobj의 stream함수는 siz=0x200을 받는다는 조건이 있어야 한다.
void fat32_add(int fld1,fat32_folder* inf,streamobj in_str)
{
	inf->set_cluster(fat32_add_sub_hide(in_str));
	insert_block(fld1,(char*)inf);
}
int floppy_outstream(streamobj* str,int,void* p)
{
	static int id1=0;
	static int id2=0;
	static int page=0;
	if(id1!=str->id)
	{
		id1=id2=str->id;
		page=0;
	}
	if(id2==0x0fffffff)
		return 0;
	readcluster(id2,page);
	if(++page==SECTOR_PER_CLUSTER)
	{
		page=0;
		id2=getcluster(id2);
	}
	if(id2==0x0fffffff)
		str->end=true;
	return 0x200;
}
//파일을 복사한다.
void fat32_cp(int fld1,int fld2,char d[11],char s[11])
{
	int nxt=fld2;
	int i;
	int j;
	fat32_find_index(fld2,d,nxt,i,j);
	readcluster(nxt,i);
	streamobj test={fld1,0,floppy_outstream};
	fat32_folder bsx=*(fat32_folder*)(FLOPPY_BUFFER+j*0x20);
	memcpy(bsx.name,s,11);
	fat32_add(fld1,&bsx,test);
}

//새 폴더를 만든다.
void fat32_new_folder(int fld1,fat32_folder* fd)
{
	int a=findempty(2);
	cluster_clean(a);
	fd->set_cluster(a);
	setcluster(a,0x0fffffff);
	insert_block(fld1,(char*)fd);
}
char* getname11(char ret[11],const std::string name)
{
	auto w=name.rfind('.');
	if(w==-1)
		w=name.length();
	memcpy(ret,name.c_str(),w<8?w:8);
	memset(ret+w,' ',8-w>0?8-w:0);
	auto tem=name.length()-w-1;
	tem=tem>0?tem:0;
	memcpy(ret+8,name.c_str()+w+1,(tem<3?tem:3));
	memset(ret+8+tem,' ',(3-tem>0)?3-tem:0);
	return ret;
}
int get_filecluster(std::string f)
{
	char name[11];
	auto w=f.find('\\');
	int start=2;
	while(true)
	{
		auto w=f.find('\\');
		if(w!=-1)f=f.substr(w+1);
		std::string df=f.substr(0,w==-1?std::string::npos:w);
		getname11(name,df.c_str());
		int clt,page,index;
		fat32_find_index(start,name,start,page,index);
		readcluster(start,page);
		start=((fat32_folder*)FLOPPY_BUFFER+0x20*index)->get_cluster();
		if(w==-1)
			return start;
	}
}/*
void native_rm(std::string f)
{
	auto w=f.rfind('\\');
	char ret[11];
	fat32_rm(get_filecluster(f.substr(0,w)),getname11(ret,f.substr(w+1)));
}*/
void native_cp(std::string a,std::string b)
{
	auto w=a.rfind('\\');
	auto x=b.rfind('\\');
	char ret1[11];
	char ret2[11];
	fat32_cp(get_filecluster(a.substr(0,w)),get_filecluster(b.substr(0,w)),getname11(ret1,a.substr(w+1)),getname11(ret1,b.substr(w+1)));
}
/*void native_mv(std::string a,std::string b)
{
	auto w=a.rfind('\\');
	auto x=b.rfind('\\');
	char ret1[11];
	char ret2[11];
	fat32_mv(get_filecluster(a.substr(0,w)),get_filecluster(b.substr(0,w)),getname11(ret1,a.substr(w+1)),getname11(ret1,b.substr(w+1)));
}*/
//시간함수 필요
void native_add(std::string a,fat32_folder* fd,streamobj s)
{
	auto w=a.rfind('\\');
	char ret1[11];
	memcpy(fd->name,a.c_str()+w+1,a.length()-w);
	fat32_add(get_filecluster(a.substr(0,w)),fd,s);
}