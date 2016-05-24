#include "provider_filesystem.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define MSWITCH( lev ) switch(ext[lev]) { default: return NULL;
#define MEND }

static char const * MIME_from_ext(char const * ext) {
	if (!ext) return NULL;
	
	again:
	
	MSWITCH (0)
	case '\0': 
		return NULL;
	case '.': 
		ext++;
		goto again;
	case 'h':
		MSWITCH (1)
		case 't':
			MSWITCH(2)
			case 'm':
				MSWITCH(3)
				case '\0':
				case 'l':
					return "text/html";
				MEND
			MEND
		MEND
	case 'j':
		MSWITCH (1)
		case 'p':
			MSWITCH (2)
			case 'e':
				MSWITCH (3)
				case 'g':
					return "image/jpeg";
				MEND
			case 'g':
				return "image/jpeg";
			MEND
		MEND
	case 'p':
		MSWITCH (1)
		case 'n':
			MSWITCH (2)
			case 'g':
				return "image/png";
			MEND
		MEND
	case 't':
		MSWITCH (1)
		case 'x':
			MSWITCH (2)
			case 't':
				return "text/plain";
			MEND
		MEND
	MEND
}

static char const * name_for_dtype(unsigned char d_type) {
	switch (d_type) {
		default:
			return "U";
		case DT_REG:
			return "F";
		case DT_DIR:
			return "D";
	}
}

static char const * class_for_dtype(unsigned char d_type) {
	switch (d_type) {
		default:
			return "unk";
		case DT_REG:
			return "file";
		case DT_DIR:
			return "dir";
	}
}

static void provider_fs_directory_view(bnex_http_request_t const * restrict req, bnex_http_response_t * restrict res, int fd, struct stat * finfo) {
	
	DIR * dir = fdopendir(fd);
	struct dirent * entry;
	
	size_t buffer_size = 128;
	size_t buffer_l = 0;
	size_t buffer_n = 0;
	char * buffer = malloc(buffer_size);
	
	char const * head = "<head><style>"
		"body{background-color:#004;margin:20px;}"
		"span{color:#fff;}"
		"span.ftype{font-weight:bold;}"
		"span.file{color:#f88;}"
		"span.dir{color:#8f8;}"
		"span.unk{color:#88f;}"
		"table{border-spacing:0px;}"
		"td{padding-left:20px;padding-right:20px;padding-top:5px;padding-bottom:5px;}"
		"a{text-decoration:none;white-space:pre;}"
		"a:link{color:#aaa;}"
		"a:visited{color:#aaa;}"
		"a:hover{color:#fff;}"
		"a:active{color:#fff;}"
		"div.main{background-color:#000;width:auto;display:inline-block;padding:20px;border-radius:20px;}"
	"</style></head><body><center><div class=\"main\"><table><tbody>";
	
	buffer_n = buffer_l + strlen(head);
	while (buffer_n > buffer_size) {
		buffer_size *= 1.5;
		buffer = realloc(buffer, buffer_size);
	}
	strcpy(buffer+buffer_l, head);
	buffer_l = buffer_n;
	
	while ((entry = readdir(dir)) != NULL) {
		if (!strcmp(entry->d_name, ".")) continue;
		char const * fmime = NULL;
		if (entry->d_type == DT_REG) {
			fmime = MIME_from_ext(strrchr(entry->d_name, '.'));
		}
		char * line = vas("<tr><td><span class=\"ftype %s\">%s</span></td><td><span>%s</span></td><td><a href=\"%s/%s\">%s</a></td></tr>", class_for_dtype(entry->d_type), name_for_dtype(entry->d_type), fmime ? fmime : "", req->path, entry->d_name, entry->d_name);
		buffer_n = buffer_l + strlen(line);
		while (buffer_n > buffer_size) {
			buffer_size *= 1.5;
			buffer = realloc(buffer, buffer_size);
		}
		strcpy(buffer+buffer_l, line);
		buffer_l = buffer_n;
	}
	
	char const * foot = "</tbody></table></div></center></body>";
	buffer_n = buffer_l + strlen(foot);
	while (buffer_n > buffer_size) {
		buffer_size *= 1.5;
		buffer = realloc(buffer, buffer_size);
	}
	strcpy(buffer+buffer_l, foot);
	buffer_l = buffer_n;
	
	bnex_http_response_set_data_buffer(res, "text/html", buffer, buffer_n);
	free(buffer);
	
	closedir(dir);
}

bool provider_fs_handle(bnex_http_request_t const * restrict req, bnex_http_response_t * restrict res) {
	
	char * path = vas("%s/%s/%s", *cwd, "webroot", req->path); // TODO -- validate path against unsafe things like '..'
	int fd = open(path, O_RDONLY);
	if (fd < 0) return false;
	
	struct stat finfo;
	fstat(fd, &finfo);
	
	if (S_ISREG(finfo.st_mode)) {
		res->code = 200;
		bnex_http_response_set_data_file(res, MIME_from_ext(strrchr(path, '.')), fd, finfo.st_size);
		return true;
	} else if (S_ISDIR(finfo.st_mode)) {
		res->code = 200;
		provider_fs_directory_view(req, res, fd, &finfo);
		return true;
	} else {
		close(fd);
		return false;
	}
}
