#include "browserdata.h"
#include "sf2_import.h"
#include <windows.h>
#include <atlbase.h>
#include <ShlObj.h>
#include "sample.h"

browserdata::browserdata()
{		
	currentmode = 0;
	IsRefreshing = false;
}

bool browserdata::init(string filename)
{	
	cachefile = filename;
	return load_cache();	
}

bool browserdata::valid_extension(int ft, const wchar_t *extension,bool &is_bank)
{
	switch(ft)
	{
	case 0:
		if ((wcsicmp(extension,L".wav") == 0)||(wcsicmp(extension,L".aif") == 0)||(wcsicmp(extension,L".aiff") == 0)/*||(wcsicmp(extension,L".rx2") == 0)*/) return true;
		break;
	case 1:
		if ((wcsicmp(extension,L".akp") == 0)||(wcsicmp(extension,L".kit") == 0)||
			(wcsicmp(extension,L".sf2") == 0)||(wcsicmp(extension,L".sfz") == 0)||
			(wcsicmp(extension,L".gig") == 0)||(wcsicmp(extension,L".dls") == 0)||
			/*(wcsicmp(extension,L".scg") == 0)||(wcsicmp(extension,L".scm") == 0)||*/
			(wcsicmp(extension,L".sc2p") == 0)||(wcsicmp(extension,L".sc2m") == 0)) 
		{
			is_bank = (wcsicmp(extension,L".sf2") == 0)||(wcsicmp(extension,L".dls") == 0)||(wcsicmp(extension,L".gig") == 0);
			return true;
		}
		break;
	};
	return false;
}

browserdata::~browserdata()
{
	
}

bool browserdata::build_samplelist(int ft, database_samplelist *s, int n_samples)
{			
	categorylist[ft].clear();
	patchlist[ft].clear();

	category_entry e;
	e.id = 0;
	sprintf(e.name,"/");
	sprintf(e.path,"");
	e.patch_id_start = 0;
	e.parent = -1;		
	e.child = 1;
	e.sibling = -1;
	e.depth = 0;
	categorylist[ft].push_back(e);
	
	for(int i=0; i<2; i++)
	{		
		category_entry es;
		es.id = categorylist[ft].size();		
		sprintf(es.path,"");
		es.patch_id_start = patchlist[ft].size();
		size_t csize = 0;

		for(int j=0; j<n_samples; j++)
		{			
			database_samplelist *sptr = (database_samplelist*)((char*)s + j*sizeof(database_samplelist));
			
			if(sptr->type != i)
			{
				patch_entry pe;
				pe.category = categorylist[ft].size();
				sprintf(pe.name,"%s",sptr->name); //,(float)sptr->size / (1024.f * 1024.f));
				sprintf(pe.column2,"%i",sptr->refcount);
				sprintf(pe.path,"loaded%i",sptr->id);
				pe.filetype = 0;
				csize += sptr->size;
				patchlist[ft].push_back(pe);
			}
		}

		es.patch_id_end = patchlist[ft].size();
		sprintf(es.name,"%s [%.1fMB]",(i==0)?"Embedded":"Referenced",(float)csize / (1024.f * 1024.f));
		
		es.parent = 0;		
		es.child = -1;
		es.sibling = (i==0)?2:-1;
		es.depth = 1;
		categorylist[ft].push_back(es);			
	}

	categorylist[ft][0].patch_id_end = patchlist[ft].size();	

	if (n_samples) *((int*)s) = 'done';

	return true;
}

void browserdata::set_paths(int type, string s)
{		
	WCHAR tmp[4096];
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, tmp, 4096);	
	paths[type] = tmp;
}

HRESULT ResolveShortcut(/*in*/ LPCWSTR lpszShortcutPath,
                        /*out*/ LPWSTR lpszFilePath)
{
    HRESULT hRes = E_FAIL;    
        // buffer that receives the null-terminated string 
        // for the drive and path
    WCHAR szPath[MAX_PATH];     
        // buffer that receives the null-terminated 
        // string for the description
    WCHAR szDesc[MAX_PATH]; 
        // structure that receives the information about the shortcut
    WIN32_FIND_DATAW wfd;    
    WCHAR wszTemp[MAX_PATH];

    lpszFilePath[0] = '\0';

	CComPtr<IShellLinkW> ipShellLink;
    // Get a pointer to the IShellLink interface
    hRes = CoCreateInstance(CLSID_ShellLink,
                            NULL, 
							CLSCTX_INPROC_SERVER,
                            IID_IShellLinkW,
                            (void**)&ipShellLink); 

    if (SUCCEEDED(hRes)) 
    { 
        // Get a pointer to the IPersistFile interface
        CComQIPtr<IPersistFile> ipPersistFile(ipShellLink);

        // IPersistFile is using LPCOLESTR, 
                // so make sure that the string is Unicode
// #if !defined _UNICODE
// 		MultiByteToWideChar(CP_UTF8, 0, lpszShortcutPath,		// var CP_ACP
//                                        -1, wszTemp, MAX_PATH);
// #else
        wcsncpy(wszTemp, lpszShortcutPath, MAX_PATH);
//#endif

        // Open the shortcut file and initialize it from its contents
        hRes = ipPersistFile->Load(wszTemp, STGM_READ); 
        if (SUCCEEDED(hRes)) 
        {
            // Try to find the target of a shortcut, 
                        // even if it has been moved or renamed
            hRes = ipShellLink->Resolve(NULL, SLR_UPDATE); 
            if (SUCCEEDED(hRes)) 
            {
                // Get the path to the shortcut target
                hRes = ipShellLink->GetPath(szPath, 
                                     MAX_PATH, &wfd, SLGP_RAWPATH); 
                if (FAILED(hRes))
                    return hRes;

                // Get the description of the target
                hRes = ipShellLink->GetDescription(szDesc,
                                             MAX_PATH); 
                if (FAILED(hRes))
                    return hRes;

                lstrcpynW(lpszFilePath, szPath, MAX_PATH); 
            } 
        } 
    } 

  return hRes;		
}

void browserdata::refresh()
{
	assert(!IsRefreshing);
	if (IsRefreshing) return;

	IsRefreshing = true;

	for(int i=0; i<max_filetypes; i++)
	{
		if(!paths[i].empty())
		{
			categorylist[i].clear();
			patchlist[i].clear();

			category_entry e;
			e.id = 0;
			sprintf(e.name,"/");
			sprintf(e.path,"");
			e.patch_id_start = 0;
			e.parent = -1;		
			e.child = -1;
			e.sibling = -1;
			e.depth = 0;
			categorylist[i].push_back(e);

			basic_string <char>::size_type a=0,b;
			int attacher = 0;

			WIN32_FIND_DATAW FindFileData;
			HANDLE hFind;
			wstring searchstring = paths[i] + L"*.lnk";

			hFind = FindFirstFileW(searchstring.c_str(), &FindFileData);		
			if (hFind != INVALID_HANDLE_VALUE) 
			{
				while(1)
				{
					WCHAR *extension = wcsrchr(FindFileData.cFileName,L'.');				
					if (extension && (wcsicmp(extension,L".lnk") == 0))
					{
						wstring lnkfile = paths[i];
						lnkfile.append(FindFileData.cFileName);

						WCHAR szFilePath[MAX_PATH];
						HRESULT hr = ResolveShortcut(lnkfile.c_str(),szFilePath);
						wcscat(szFilePath,L"\\");

						if(!FAILED(hr)) attacher = traverse_dir(i,szFilePath,1,attacher,0);

					}
					if (!FindNextFileW(hFind,&FindFileData)) break;			
				}
				FindClose(hFind);
			}
			categorylist[i][0].patch_id_end = patchlist[i].size();	
		}
	}
	save_cache();

	IsRefreshing = false;
}

bool browserdata::inject_newfile(int type, wstring filename)
{
	wstring path = filename.substr(0,filename.rfind('\\')+1);		
	char fileUTF8[256];
	char pathUTF8[256];
	WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, pathUTF8, 256, 0,0);	
	WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, fileUTF8, 256, 0,0);	

	UINT n = categorylist[type].size();
	for(int c=0; c<n; c++)
	{
		if(_strnicmp(categorylist[type][c].path,pathUTF8,256) == 0)
		{
			// sort the patches within the category
			// just need to choose the correct pid when inserting to do it
			int pid = categorylist[type][c].patch_id_start;
			while(pid < (categorylist[type][c].patch_id_end-1))
			{
				int r = _strnicmp(fileUTF8,patchlist[type][pid].path,256);
				if(r<0) break;
				if(r == 0) return false;	// already exists
				pid++;
			}			

			patch_entry e;
			e.category = c;			
			WideCharToMultiByte(CP_UTF8, 0, filename.c_str(), -1, e.path, 256, 0,0);					
			wstring name = filename.substr(filename.rfind('\\')+1);
			name = name.substr(0,name.rfind('.'));
			WideCharToMultiByte(CP_ACP, 0, name.c_str(), -1, e.name, 64, 0,0); 
					
			patchlist[type].insert(patchlist[type].begin() + pid,e);			
			
			// offset categorylist to reflect
			categorylist[type][c].patch_id_end++;
			// offset all parents end_id if needed
			int cid = categorylist[type][c].parent;
			while((cid>=0)&&(cid<categorylist[type].size()))
			{
				categorylist[type][cid].patch_id_end = max(categorylist[type][cid].patch_id_end,categorylist[type][c].patch_id_end);
				cid = categorylist[type][cid].parent;
			}
			// offset following categories
			for(int i=c+1; i<n; i++)
			{
				categorylist[type][i].patch_id_start++;
				categorylist[type][i].patch_id_end++;
			}

			save_cache();
			return true;
		}
	}
	return false;
}

// move somewhere else
void Trim(std::string& str, const std::string & ChrsToTrim = " \t\n\r", int TrimDir = 0)
{
	size_t startIndex = str.find_first_not_of(ChrsToTrim);
	if (startIndex == std::string::npos){str.erase(); return;}
	if (TrimDir < 2) str = str.substr(startIndex, str.size()-startIndex);
	if (TrimDir!=1) str = str.substr(0, str.find_last_not_of(ChrsToTrim) + 1);
}

inline void TrimRight(std::string& str, const std::string & ChrsToTrim = " \t\n\r")
{
	Trim(str, ChrsToTrim, 2);
}

inline void TrimLeft(std::string& str, const std::string & ChrsToTrim = " \t\n\r")
{
	Trim(str, ChrsToTrim, 1);
}

int browserdata::traverse_dir(int ftype, wstring dir, int depth, int creator, int parent)
{			
	assert(depth<16);
		
	category_entry e;
	wstring name = dir;
	name.erase(name.length()-1);
	name.erase(0,name.rfind('\\')+1);
	//strncpy(e.name,name.c_str(),64);
	WideCharToMultiByte(CP_ACP, 0, name.c_str(), -1, e.name, 64, 0,0);	
	WideCharToMultiByte(CP_UTF8, 0, dir.c_str(), -1, e.path, 256, 0,0);	
	e.id = categorylist[ftype].size();		
	e.parent = parent;
	e.child = -1;
	e.sibling = -1;
	if(categorylist[ftype][creator].depth == depth) categorylist[ftype][creator].sibling = e.id;
	else categorylist[ftype][creator].child = e.id;	
	e.depth = depth;
	e.patch_id_start = patchlist[ftype].size();	
	
	categorylist[ftype].push_back(e);	
	
	int subcreator = e.id;

	WCHAR searchdir[512];
	swprintf(searchdir,L"%s*",dir.c_str());	
	
	WIN32_FIND_DATAW FindFileData;
	HANDLE hFind;
		
	hFind = FindFirstFileW(searchdir, &FindFileData);		
	if (hFind != INVALID_HANDLE_VALUE) 
	{
		while(1)
		{						
			if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (FindFileData.cFileName[0] == '.'))
			{
				// . and ..
				// do nothing
			}
			else if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{				
				wstring subdir = dir;				
				subdir.append(FindFileData.cFileName);
				subdir.append(L"\\");
				subcreator = traverse_dir(ftype,subdir,depth+1,subcreator,e.id);				
			} 
			else
			{
				WCHAR *extension = wcsrchr(FindFileData.cFileName,L'.');				
				if (extension)
				{	
					bool is_bank=false;					
					if(valid_extension(ftype,extension,is_bank))
					{
						if(is_bank)
						{
							wstring sf2file = dir;
							sf2file.append(FindFileData.cFileName);
							midipatch *plist=0;
							int n;
							if(wcsicmp(extension,L".sf2") == 0) n = get_sf2_patchlist(sf2file.c_str(), (void**)&plist);
							else n = get_dls_patchlist(sf2file.c_str(), (void**)&plist);
							for(int i=0; i<n; i++)
							{
								patch_entry f;						
								WCHAR s[256];
								swprintf(s,L"%i",i);
								wstring spath;
								if((n>1) && (i>0)) spath = dir + FindFileData.cFileName + L">" + s;
								else spath = dir + FindFileData.cFileName;								
								WideCharToMultiByte(CP_UTF8, 0, spath.c_str(), -1, f.path, 256, 0,0); 
								f.category = e.id;							

								//f.path.append(i);	

								string pname = plist[i].name;
								// remove leading/trailing white space
								Trim(pname);
								vtCopyString(f.name,pname.c_str(),64);
								f.filetype = ftype;								
								WideCharToMultiByte(CP_UTF8, 0, extension, -1, f.column2, 16, 0,0);		_strlwr_s(f.column2,16);								
								patchlist[ftype].push_back(f);								
							}
							delete plist;
						}
						else
						{
							patch_entry f;		
							wstring tmp = dir + FindFileData.cFileName;
							//strncpy(f.path,tmp.c_str(),256);
							WideCharToMultiByte(CP_UTF8, 0, tmp.c_str(), -1, f.path, 256, 0,0); 
							f.category = e.id;
							f.filetype = ftype;
							WideCharToMultiByte(CP_UTF8, 0, extension, -1, f.column2, 16, 0,0);		_strlwr_s(f.column2,16);							
							*extension = 0;
							//strncpy(f.name,FindFileData.cFileName,64);
							WideCharToMultiByte(CP_ACP, 0,FindFileData.cFileName, -1, f.name, 64, 0,0); 		
							patchlist[ftype].push_back(f);							
						}
					} 					
				}
			}

			if (!FindNextFileW(hFind,&FindFileData)) break;			
		}
		FindClose(hFind);	
	}	
	categorylist[ftype][e.id].patch_id_end = patchlist[ftype].size();	

	// remove category from list if it doesn't contain any patches
	/*if(depth && (categorylist[ftype][e.id].patch_id_end == categorylist[ftype][e.id].patch_id_start))
	{
		//parent->subcategories.erase(parent->subcategories.end()-1);
		//delete ep;
		//categorylist[ftype][creator].child = -1;
		categorylist[ftype].erase(categorylist[ftype].begin()+e.id);
	}*/
	return e.id;
}

void browserdata::save_cache()
{
	FILE *f = fopen(cachefile.c_str(),"wb");
	
	int i;
	i = categorylist[0].size(); fwrite(&i,4,1,f);	
	i = patchlist[0].size(); fwrite(&i,4,1,f);
	
	i = categorylist[1].size(); fwrite(&i,4,1,f);	
	i = patchlist[1].size(); fwrite(&i,4,1,f);
	
	i = categorylist[2].size(); fwrite(&i,4,1,f);	
	i = patchlist[2].size(); fwrite(&i,4,1,f);
	
	i = categorylist[3].size(); fwrite(&i,4,1,f);
	i = patchlist[3].size(); fwrite(&i,4,1,f);

	for(int s=0; s<4; s++)
	{
		for(int e=0; e<categorylist[s].size(); e++)
		{
			category_entry ce = categorylist[s][e];
			fwrite(&ce,sizeof(category_entry),1,f);			
		}
		for(int e=0; e<patchlist[s].size(); e++)
		{
			patch_entry pe = patchlist[s][e];
			fwrite(&pe,sizeof(patch_entry),1,f);
		}
	}	

	fclose(f);	
}

bool browserdata::load_cache()
{
	FILE *f = fopen(cachefile.c_str(),"rb");

	if(!f) return false;
	
	int nc[4],np[4];
	for(int s=0; s<4; s++)
	{
		if(!fread(&nc[s],4,1,f)) return false;
		if(!fread(&np[s],4,1,f)) return false;
	}
	
	for(int s=0; s<4; s++)
	{		
		categorylist[s].clear();
		patchlist[s].clear();

		for(int e=0; e<nc[s]; e++)
		{
			category_entry ce;
			if (!fread(&ce,sizeof(category_entry),1,f)) return false;	
			categorylist[s].push_back(ce);
		}
		for(int e=0; e<np[s]; e++)
		{
			patch_entry pe;
			if(!fread(&pe,sizeof(patch_entry),1,f)) return false;
			patchlist[s].push_back(pe);
		}		
	}	

	fclose(f);	

	return true;
}