#include "linux_aux_wrapper.h"
#include "opengl1.h"

#ifdef __linux__
#include <dlfcn.h>
#include <time.h>
#include <dirent.h> 

DWORD timeGetTime() {
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		clock_gettime(CLOCK_REALTIME, &ts);
	}

	DWORD secs = ts.tv_sec;
	secs *= 1000;
	DWORD nsecs = ts.tv_nsec;
	nsecs /= 1000000;
	return secs + nsecs;
}

void Sleep(DWORD millis) {
	usleep(millis * 1000);
}

int GetSystemMetrics(int nIndex) {
	
 
	 if (nIndex == SM_CXSCREEN) {
		 int sc = WidthOfScreen(XScreenOfDisplay(glfwGetX11Display(), 0));
		 return sc;
	 }
	 else if (nIndex == SM_CYSCREEN) {
		 int sc = HeightOfScreen(XScreenOfDisplay(glfwGetX11Display(), 0));
		 return sc;
	 }
	 else {
		 printf("unimplemented GetSystemMetrics\n");
		 exit(0);
	 }
}

BOOL SetCurrentDirectory(LPCSTR lpPathName) {
	char tmp[1024];
	
	strcpy(tmp, lpPathName);
	char * t = tmp;
	while (*t) {
		if (*t == '\\') {
			*t = '/';
		}
		t++;
	}
	
	chdir(tmp);

	return 1;
}

int fseek1(FILE * fp, int a, int b) {
	if (b == SEEK_CUR) {
		if (a == -1) {
			int c = fgetc(fp);
			if (c == '\n') {
				fseek(fp, -1, SEEK_CUR);
			}
			else {
				fseek(fp, -2, SEEK_CUR);
			}
			return 0;
		}
		
	}
	return fseek(fp, a, b);
}

FILE * (*native_fopen)(const char*, const char*) = NULL;

FILE * fopen(const char * fname, const char * mode) {

	if (!native_fopen) {
		native_fopen = (FILE * (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen");
	}
	char tmp[1024];
	strcpy(tmp, fname);
	char * t = tmp;
	char * tpos = NULL;
	const char * dir;
	while (*t) {
		if (*t == '/' || *t == '\\') {
			tpos = t;
		}
		t++;
	}
	if (tpos) {
		*tpos = 0;
		tpos++;		
		dir = tmp;		
	}
	else {
		dir = ".";
		tpos = tmp;
	}
	
	DIR *d;
	struct dirent *dire;
	d = opendir(dir);
	char target_name[1024] = { 0, };
	if (d) {
		while ((dire = readdir(d)) != NULL) {
			if (dire->d_type != DT_REG) {
				continue;
			}
			if (!strcasecmp(dire->d_name, tpos)) {				
				strcpy(target_name,dire->d_name);
				break;
			}
		}
		closedir(d);
	}

	// ponytail: separate buffer - dir aliases tmp, and glibc's snprintf clears the
	// destination before reading the arguments, so formatting into tmp turned every
	// absolute path into "/<basename>".
	char located[1024];

	if (!target_name[0]) {
		if (mode[0] == 'r') {
			return NULL;
		} else {
			// nothing to match against, create the file exactly as asked
			strcpy(located, fname);
		}
	}
	else {
		int err = snprintf(located, sizeof(located), "%s/%s",dir,target_name);
		if ((err < 0) || (size_t(err) >= sizeof(located))) {
			return NULL;
		}
	}

	//printf("located %s for %s\n", located, fname);


	return native_fopen(located, mode);
}

#endif