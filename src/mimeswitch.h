#define CC(chr) case chr: case chr-32:
#define MSWITCH( lev ) switch(ext[lev]) { default: return NULL;
#define MEND }

static inline char const * MIME_from_ext(char const * ext) {
	if (!ext) return NULL;
	
	again:
	
	MSWITCH (0)
	case '\0': 
		return NULL;
	case '.': 
		ext++;
		goto again;
	CC('c')
		MSWITCH (1)
		CC('s')
			MSWITCH (2)
			CC('s')
				return "text/css";
			MEND
		MEND
	CC('f')
		MSWITCH (1)
		CC('l')
			MSWITCH(2)
			CC('a')
				MSWITCH(3)
				CC('c')
					return "audio/flac";
				MEND
			MEND
		MEND
	CC('g')
		MSWITCH (1)
		CC('i')
			MSWITCH (2)
			CC('f')
				return "image/gif";
			MEND
		MEND
	CC('h')
		MSWITCH (1)
		CC('t')
			MSWITCH(2)
			CC('m')
				MSWITCH(3)
				case '\0':
				CC('l')
					return "text/html";
				MEND
			MEND
		MEND
	CC('i')
		MSWITCH (1)
		CC('c')
			MSWITCH (2)
			CC('o')
				return "image/x-icon";
			MEND
		MEND
	CC('j')
		MSWITCH (1)
		CC('p')
			MSWITCH (2)
			CC('e')
				MSWITCH (3)
				CC('g')
					return "image/jpeg";
				MEND
			CC('g')
				return "image/jpeg";
			MEND
		MEND
	CC('m')
		MSWITCH (1)
		CC('p')
			MSWITCH (2)
			CC('3')
				return "audio/mpeg";
			MEND
		MEND
	CC('p')
		MSWITCH (1)
		CC('n')
			MSWITCH (2)
			CC('g')
				return "image/png";
			MEND
		MEND
	CC('t')
		MSWITCH (1)
		CC('x')
			MSWITCH (2)
			CC('t')
				return "text/plain";
			MEND
		MEND
	CC('w')
		MSWITCH (1)
		CC('e')
			MSWITCH (2)
			CC('b')
				MSWITCH (3)
				CC('m')
					return "video/webm";
				MEND
			MEND
		MEND
	MEND
}
