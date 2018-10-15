inline int readfat1(int cluster,int bp)
{
	auto wc=ftell(o);
	fseek(o,(BPB_RESERVED_SECTOR+BPB_HIDDEN_SECTOR)*BPB_BYTE_PER_SECTOR*SECTOR_PER_CLUSTER+cluster*4,0);
	*(int*)buf=bp;
	fread(buf,4,1,o);
	fseek(o,wc,0);
	return *(int*)buf;
}
inline void writefat1(int cluster,int bp)
{
	auto wc=ftell(o);
	fseek(o,(BPB_RESERVED_SECTOR+BPB_HIDDEN_SECTOR)*BPB_BYTE_PER_SECTOR*SECTOR_PER_CLUSTER+cluster*4,0);
	*(int*)buf=bp;
	fwrite(buf,4,1,o);
	fseek(o,wc,0);
}
inline int readcluster(int cluster,int offset,char*buff,int l)
{
	auto wc=ftell(o);
	fseek(o,(cluster-BPB_ROOT_ENTRY_CLUSTER+FAT_NUM*BPB_FAT32_SIZE+BPB_RESERVED_SECTOR+BPB_HIDDEN_SECTOR)*BPB_BYTE_PER_SECTOR*SECTOR_PER_CLUSTER+offset,0);
	int ret=fread(buff,l,1,o);
	fseek(o,wc,0);
	return ret;
}
inline void writecluster(int cluster,int offset,char*buff,int l)
{
	auto wc=ftell(o);
	fseek(o,(cluster-BPB_ROOT_ENTRY_CLUSTER+FAT_NUM*BPB_FAT32_SIZE+BPB_RESERVED_SECTOR+BPB_HIDDEN_SECTOR)*BPB_BYTE_PER_SECTOR*SECTOR_PER_CLUSTER+offset,0);
	fwrite(buff,l,1,o);
	fseek(o,wc,0);
}
inline int findemptycluster(int startcluster)
{
	auto wc=ftell(o);
	fseek(o,(BPB_RESERVED_SECTOR+BPB_HIDDEN_SECTOR)*BPB_BYTE_PER_SECTOR*SECTOR_PER_CLUSTER+startcluster*4,0);
	while(true)
	{
		if(startcluster>BPB_FAT32_SIZE*BPB_BYTE_PER_SECTOR*SECTOR_PER_CLUSTER/4)
		{
			printf ("%s","disk is full\n");
			exit(1);
		}
		fseek(o,(BPB_RESERVED_SECTOR+BPB_HIDDEN_SECTOR)*BPB_BYTE_PER_SECTOR*SECTOR_PER_CLUSTER+startcluster*4,0);
		fread(buf,4,1,o);
		if(!*(int*)buf)
			break;
		startcluster++;
	}
	fseek(o,wc,0);
	return startcluster;
}
inline void filewrite(int startcluster,string filename,int len)
{
	FILE* k;
	error1(fopen_s(&k,filename.c_str(),"rb"),"access denied");
#define repd (len>>9)
#define repm (len&0x1ff)
#define red (flen>>9)
#define rem (flen&0x1FF)
	int flen;
	while(true)
	{
		flen=(BPB_BYTE_PER_SECTOR*SECTOR_PER_CLUSTER);
		int i=0;
		for(;red&&repd;i++)
		{
			fread(buf,512,1,k);
			writecluster(startcluster,i*512,buf,512);
			len-=512;
			flen-=512;
		}
		if(!flen||!len);
		else if(flen<len)
		{
		fread(buf,1,flen,k);
		len-=flen;
		}
		else
		{
		fread(buf,1,len,k);
		len=0;
		}
		writecluster(startcluster,i*512,buf,flen<len?flen:len);
		if(!len)
		{
			writefat1(startcluster,0x0FFFFFFF);
			return;
		}
		//새 클러스터를 찾는다
		int cnc=findemptycluster(startcluster+1);
		writefat1(startcluster,cnc);
		startcluster=cnc;
	}
	return;
}
#undef repd
#undef repm
#undef red
#undef rem