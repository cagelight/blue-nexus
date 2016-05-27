#include "provider_filesystem.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "mimeswitch.h"

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
			return "u";
		case DT_REG:
			return "f";
		case DT_DIR:
			return "d";
	}
}

static void provider_fs_directory_view(bnex_http_request_t const * restrict req, bnex_http_response_t * restrict res, int fd, struct stat * finfo) {
	
	uint_fast16_t tralt = 0;
	
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
		"span.f{color:#f88;}"
		"span.d{color:#8f8;}"
		"span.u{color:#88f;}"
		"table{border-spacing:0px;}"
		"td{padding-left:20px;padding-right:20px;padding-top:5px;padding-bottom:5px;}"
		"tr{background-color:#080808;}"
		"tr.off{background-color:#0C0C0C;}"
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
		char * line = vas("<%s><td><span class=\"ftype %s\">%s</span></td><td><span>%s</span></td><td><a href=\"%s/%s\">%s</a></td></tr>", tralt++ % 2 ? "tr class = \"off\"" : "tr", class_for_dtype(entry->d_type), name_for_dtype(entry->d_type), fmime ? fmime : "", req->path, entry->d_name, entry->d_name);
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
		struct tm ims_tm = req->if_modified_since;
		if (req->has_ims) {
			double diff = difftime(finfo.st_mtim.tv_sec, mktime(&ims_tm));
			com_printf_info("%f", diff);
			if (diff <= 0) {
				res->code = 304;
				return true;
			}
		}
		struct tm date;
		localtime_r(&finfo.st_mtim.tv_sec, &date);
		res->code = 200;
		char * date_str = vas_next();
		strftime(date_str, VAS_BUFLEN, "%a, %d %b %Y %T %Z", &date);
		bnex_http_response_set_add_field(res, "Last-Modified", date_str);
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
