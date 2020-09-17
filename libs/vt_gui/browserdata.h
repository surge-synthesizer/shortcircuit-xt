#pragma once
#include <string>
#include <vector>
#include <assert.h>
using namespace std;

struct patch_entry
{
	char name[64];
	char path[256];
	char column2[16];
	int category;
	int filetype;
	bool favourite;
};

struct category_entry
{
	char name[64];
	char path[256];
	int id,depth;
	int patch_id_start,patch_id_end;	
	int child,sibling,parent;	
};

struct database_samplelist
{
	char name[64];
	int id;
	size_t size;
	int refcount;
	int type; // hddref = 0, embedded = 1...	
};

const int max_filetypes = 4;

class browserdata
{
public:
	browserdata();
	~browserdata();
	
	void set_paths(int type, string s); // semi-colon seperated for multiple directories		
	void refresh();
	void save_cache();
	bool load_cache();
	bool init(string filename);
	bool inject_newfile(int type, wstring filename);
	bool build_samplelist(int ft, database_samplelist *s, int n_samples);

	vector<patch_entry>		patchlist[max_filetypes];		
	vector<category_entry>	categorylist[max_filetypes];

	int currentmode;

private:	
	int traverse_dir(int ftype, wstring dir, int depth, int attacher, int parent);	
	bool valid_extension(int,const wchar_t*,bool &is_bank);	
	vector<string> ft_ext[max_filetypes];
	wstring paths[max_filetypes];
	string cachefile;	
	bool IsRefreshing;
};