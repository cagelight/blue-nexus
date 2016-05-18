#include "provider_filesystem.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
	
	return NULL;
}

bool provider_fs_handle(bnex_http_request_t const * restrict req, bnex_http_response_t * restrict res) {
	
	char * path = vas("%s/%s/%s", *cwd, "webroot", req->path); // TODO -- validate path against unsafe things like '..'
	int fd = open(path, O_RDONLY);
	if (fd < 0) return false;
	
	struct stat finfo;
	fstat(fd, &finfo);
	
	if (!S_ISREG(finfo.st_mode)) {
		close(fd);
		return false;
	}
	
	res->code = 200;
	
	bnex_http_response_set_data_file(res, MIME_from_ext(strrchr(path, '.')), fd, finfo.st_size);
	
	return true;
}
