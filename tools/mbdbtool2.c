/**
  * libmbdb-1.0 - mbdbtool2.c
  * Copyright (C) 2013 - A. Gordon
  *
  * Based on mbdbtool.c:
  * Copyright (C) 2013 Crippy-Dev Team
  * Copyright (C) 2011-2013 Joshua Hill
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <err.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <dirent.h>
#include <time.h>

#include <openssl/sha.h>

#include <libmbdb-1.0/backup.h>
#include <libcrippy-1.0/libcrippy.h>

enum DOMAIN_TYPE
{
	LIST_SYSTEM_DOMAINS,
	LIST_APP_DOMAINS
};

/* Available commands */
enum MBDB_COMMANDS
{
	CMD_UNKNOWN,
	CMD_DUMP_MBDB,
	CMD_LIST_FILES,
	CMD_LIST_DOMAINS,
	CMD_LIST_APPS,
	CMD_LIST_CAMERA_ROLL,
	CMD_MBDB_INFO
};

struct {
	enum MBDB_COMMANDS cmd;
	char* name;
	char* description;
} const command_names[] = {
	{CMD_DUMP_MBDB,   "dump", "dump MBDB recods (technical values)"},
	{CMD_LIST_FILES,  "ls",    "list MBDB files"},
	{CMD_LIST_DOMAINS,"doms",  "list MBDB system domains"},
	{CMD_LIST_APPS,   "apps",  "list MBDB applications"},
	{CMD_LIST_CAMERA_ROLL,"cam",   "list Camera Roll images"},
	{CMD_UNKNOWN,     NULL,    NULL}
};

#define IS_MODE_SYMLINK(x)   (((x)&0xE000)==0xA000)
#define IS_MODE_FILE(x)      (((x)&0xE000)==0x8000)
#define IS_MODE_DIRECTORY(x) (((x)&0xE000)==0x4000)

/* Global Variables */
char *backup_parent_directory = NULL ;
char *backup_directory = NULL ;
char *udid = NULL ;
enum MBDB_COMMANDS command;

/* Command line options */
const struct option mbdb_options[] = {
	{"help",	no_argument,	0,	'h'},
	{0,		0,		0,	0}
};

/* Show a usage-related error message,
   with additional "try --help" message,
   then exit with exit code 1 */
void usage_error(const char *error_message,...)
{
	if (error_message!=NULL && strlen(error_message)>0) {
		va_list ap;
		va_start(ap, error_message);
		vfprintf(stderr,error_message,ap);
		va_end(ap);
	}
	fprintf(stderr,"Try 'mbdbtool2 --help' for more information\n");
	exit(EXIT_FAILURE);
}


/* Given a domain (e.g. "CameraRollDomain")
   and a file path, calculates the SHA1 hash of the filename.

   SHA1 output is stored in "sha1" variable, which MUST have at least 20 bytes allocated.
   No error checking is performed. */
void sha1_filename(const char* domain, const char* path,
		char* /*OUTPUT*/ sha1)
{
	SHA_CTX shactx;
	SHA1_Init(&shactx);
	SHA1_Update(&shactx, domain, strlen(domain));
	SHA1_Update(&shactx, "-", 1);
	SHA1_Update(&shactx, path, strlen(path));
	SHA1_Final(sha1, &shactx);
}

/* Prints a HexDump of input buffer 'data' to STDOUT */
void hexdump_buffer(const unsigned char *data, size_t len)
{
	size_t i;
	for (i = 0; i < len; i++) {
		printf("%02x", data[i]);
	}
}

/* Show detailed help screen  */
void show_help()
{
	int i=0;

	printf(\
"mbdbtool2 - iOS Mbdb File Parser\n" \
"\n" \
"Usage: mbdbtool2 [OPTIONS] DIR UDID CMD\n" \
"\n" \
"Required Parameters:\n" \
"  DIR  - The backup directory\n" \
"  UDID - The iDevice UDID (40 characters)\n" \
"         The file 'DIR/UDID/Manifest.mbdb' must exist.\n" \
"  CMD  - Command/Action to perform:\n");
	while (command_names[i].name != NULL) {
		printf("    %-5s - %s\n",
				command_names[i].name,
				command_names[i].description);
		++i;
	}
	printf("\n");
	printf(\
"Examples:\n" \
" # List MBDB files\n" \
" $ mbdbtool2 ~/iphone_backup/ c7030a299c4e61e0ef6f61b0f3ddf45111111111 list\n" \
" drwxr-xr-x            0 e31f831d5a892f05fc0e319d26f1761287fd88a6 AppDomain-com.apple.itunesu  Library\n" \
" drwxr-xr-x            0 4952e822da9ed011c90095ae9df9ec4707fe85ca AppDomain-com.apple.itunesu  Library/Preferences\n" \
" lrwxr-xr-x            0 192de30ac28708ba014fca830d8ac44bb74ae052 AppDomain-com.apple.itunesu  Library/Preferences/com.apple.PeoplePicker.plist -> /private/var/mobile/Library/Preferences/com.apple.PeoplePicker.plist\n" \
" ...\n" \
"\n" \
" # List Camera Roll Images\n" \
" $ mbdbtool2 ~/iphone_backup/ c7030a299c4e61e0ef6f61b0f3ddf45111111111 cam\n" \
" 343e26971dfe9c395c425c0ccf799df63ae6261e Media/DCIM/100APPLE/IMG_0001.JPG IMG_0001.JPG JPG 2013/06/14 02:16:55\n" \
" d46c91af097d8ea8c13f56bb0ea882325dc7d0e9 Media/DCIM/100APPLE/IMG_0002.JPG IMG_0002.JPG JPG 2013/06/14 02:17:51\n" \
" d27997f2fbefc0ad9027c9b91239154266859e53 Media/DCIM/100APPLE/IMG_0003.JPG IMG_0003.JPG JPG 2013/06/14 21:32:00\n" \
" 9e3156b537de1cea17a389fd1c9faaa8cb870701 Media/DCIM/100APPLE/IMG_0004.JPG IMG_0004.JPG JPG 2013/06/14 21:33:30\n" \
" ...\n" \
"\n" \
" # List Applications\n" \
" $ mbdbtool2 ~/iphone_backup/ c7030a299c4e61e0ef6f61b0f3ddf45111111111 apps\n" \
" com.apple.WebViewService\n" \
" com.google.Translate\n" \
" com.pandora\n" \
" com.popcap.ios.BWH\n" \
" com.skype.skype\n" \
" ...\n" \
"\n"
	);
}

/* Returns NON-ZERO if 'dir' is a valid directory */
int is_valid_directory(const char* dir)
{
	struct stat sb;
	return ((stat(dir, &sb) == 0 && S_ISDIR(sb.st_mode)));
}

/* Returns NON-ZERO if 'udid' looks like valid UDID:
    40 hexadecimal characters */
int is_valid_udid(const char* udid)
{
	return udid
		&&
		strlen(udid)==40
		&&
		strspn(udid,"0123456789ABCDEFabcdef")==40;
}

/* Validate the DIRECTORY and UDID parameters,
   the setup the corresponding global variables.

   The function always succeeds, or terminates on error.*/
void setup_DIR_UDID(const char *dir, const char* _udid)
{
	if ( !is_valid_directory(dir) )
		errx(1, "error: '%s' is not a directory", dir);
	if ( !is_valid_udid(_udid) )
		errx(1, "error: '%s' is not a valid UDID", _udid);

	backup_parent_directory = strdup(dir);
	if (backup_parent_directory==NULL)
		err(1,"strdup failed");
	udid = strdup(_udid);
	if (udid==NULL)
		err(1,"strdup failed");

	backup_directory = malloc(strlen(backup_parent_directory)+1
			         +
				 strlen(udid)+1);
	if (backup_directory==NULL)
		err(1,"malloc failed");
	strcpy(backup_directory,backup_parent_directory);
	strcat(backup_directory,"/");
	strcat(backup_directory,udid);
	if (!is_valid_directory(backup_directory))
		errx(1,"error: Combined DIR/UDID directory " \
			"'%s' is not a directory",backup_directory);
}

/* Validates the CMD (=command) parameter,
   and sets the global 'command' variable accordingly.

   The function always succeeds, or terminates on error.*/
void setup_command(const char*cmd)
{
	int i=0;

	if (cmd==NULL || strlen(cmd)==0)
		usage_error("error: missing command\n");

	while (command_names[i].name != NULL) {
		if (strlen(cmd)==strlen(command_names[i].name)
			&&
		    strcmp(cmd,command_names[i].name)==0) {
			command = command_names[i].cmd;
			return;
		}
		++i;
	}

	usage_error("error: unknown command '%s'\n", cmd);
}

/* Parses the command-line arguments,
   sets the backup directory, UDID and command variables.

   The function always succeeds, or terminates on error.*/
void parse_command_line(int argc, char* argv[])
{
	int c,i;
	while ( (c=getopt_long(argc,argv,"h",mbdb_options,&i)) != -1) {
		switch (c)
		{
		case 'h':
			show_help();
			exit(0);
			break;
		}
	}

	/* We require at least three extra parameters: DIR UDID CMD */
	if ( optind + 3 > argc )
		usage_error("Missing 3 required parameters (DIR UDID CMD).\n");

	setup_DIR_UDID(argv[optind],argv[optind+1]);

	setup_command(argv[optind+2]);
}

/* Returns a pointer to the iOS/MBDB string,
   or to a static string indicating that the input iOS string was empty.

   Empty strings in MBDB are denoted by either size 0 or size 65535 (0xFFFF)*/
const char* ios_string(unsigned short size, const char*str)
{
	if (size==0 || size==0xFFFF)
		return "()";
	if (str==NULL)
		return "(?)"; //this should not happen
	return str;
}

/* Dumps the content of the MBDB records to STDOUT.

   Every variable of each record is printed, in hex/octal/hexdump as needed. */
void dump_mbdb(backup_t* backup)
{
	int i;
	int file_counts = backup_get_num_files(backup);
	if (file_counts<=0)
		return;

	printf("RecNum\t" \
	       "sha1\t" \
	       "Domain\t" \
	       "Path\t" \
	       "Target\t" \
	       "Datahash\t" \
	       "Unknown1\t" \
	       "Mode\t" \
	       "Unknown2\t" \
	       "inode\t" \
	       "uid\t" \
	       "gid\t" \
	       "mtime\t" \
	       "atime\t" \
	       "ctime\t" \
	       "filelen\t" \
	       "flags\t" \
	       "NumProps"
	       );
	printf("\n");
	for (i = 0 ; i < file_counts ; ++i) {
		backup_file_t* file = backup_get_file_by_index(backup, i);

		const mbdb_record_t *m = file->mbdb_record;

		printf("%d\t",i);
		char sha1[20];

		if (IS_MODE_FILE(m->mode)) {
			sha1_filename(m->domain, m->path, sha1);
			hexdump_buffer(sha1,20);
		} else {
			printf("()");
		}
		putc(' ', stdout);
		printf("%s\t",ios_string(m->domain_size, m->domain));
		printf("%s\t",ios_string(m->path_size, m->path));
		printf("%s\t",ios_string(m->target_size, m->target));
		if (m->datahash_size==0 || m->datahash_size==65535)
			printf("()\t");
		else
			hexdump_buffer(m->datahash,m->datahash_size);
		printf("%s\t",ios_string(m->unknown1_size, m->unknown1));
		printf("0x%x\t",m->mode);
		printf("0x%d\t",m->unknown2);
		printf("0x%x\t",m->inode);
		printf("0%o\t",m->uid);
		printf("0%o\t",m->gid);
		printf("%lld\t",m->length);
		printf("0x%x\t",m->flag);
		printf("%d",m->property_count);
		printf("\n");

		//Segfault..
		//backup_file_free(file_t);
	}
}

/* Prints a user-friendly "ls"-like listing of files and directories in the
   MBDB file.
   Only selected fields are printed:
      SHA1 hash (calculated, not in MBDB),
      Domain, File Path, File type (directory, file, symlink),
      size, file-mode, etc. */
void list_backup_files(backup_t* backup)
{
	int i;
	int file_counts = backup_get_num_files(backup);
	if (file_counts<=0)
		return;

	for (i = 0 ; i < file_counts ; ++i) {
		backup_file_t* file = backup_get_file_by_index(backup, i);

		const mbdb_record_t *m = file->mbdb_record;

		/* A Domain record? not a file... */
		if (m->path_size==0 || m->path_size==65535)
			continue;

		/* Print type character */
		if (IS_MODE_SYMLINK(m->mode))
			putc('l',stdout);
		else
		if (IS_MODE_DIRECTORY(m->mode))
			putc('d',stdout);
		else
		if (IS_MODE_FILE(m->mode))
			putc('-',stdout);
		else
			errx(1,"error in MBDB record %d, unknown 'mode' (0x%x)", i, m->mode);

		/* Print file mode */
		char modestr[10] = "rwxrwxrwx";
		int idx,bit;
		for (idx=0, bit=8; idx<9;++idx,--bit) {
			const unsigned short mo = m->mode & 0xFFF;
			const short v = (mo>>bit)&0x1;
			if (v==0)
				modestr[idx] = '-';
		}
		printf("%s ", modestr);

		printf("%12lld ", m->length);

		char sha1[20];
		sha1_filename(m->domain, m->path, sha1);
		hexdump_buffer(sha1,20);

		printf(" %s ",ios_string(m->domain_size, m->domain));
		printf(" %s",ios_string(m->path_size, m->path));

		if (IS_MODE_SYMLINK(m->mode)) {
			if (m->target_size==0 || m->target_size==65535)
				errx(1,"error in MBDB record %s, symlink has empty target", i);
			printf(" -> %s",m->target);
		}

		printf("\n");

		//Segfault..
		//backup_file_free(file_t);
	}
}


/* Utility function to by used with "qosrt", copied from qsort(3) man page. */
static int
cmpstringp(const void *p1, const void *p2)
{
	/* The actual arguments to this function are "pointers to
	   pointers to char", but strcmp(3) arguments are "pointers
	   to char", hence the following cast plus dereference */

	return strcmp(* (char * const *) p1, * (char * const *) p2);
}


/* Prints a sorted list of domains in the MBDB file.

   Based on "type", prints either the system domains (e.g. "CameraRollDomain"),
   or the application domains (e.g. the installed applications). */
void list_domains(backup_t* backup, enum DOMAIN_TYPE type)
{
	int i;
	int file_counts = backup_get_num_files(backup);
	if (file_counts<=0)
		return;

	/* There can't be more domains than records - so this is safe */
	int domains_count=0;
	char **domains = (char**)malloc(sizeof(char*) * file_counts);
	if (domains==NULL)
		errx(1,"malloc(domains) failed");

	for (i = 0 ; i < file_counts ; ++i) {
		backup_file_t* file = backup_get_file_by_index(backup, i);

		const mbdb_record_t *m = file->mbdb_record;

		/* A Domain/App record - must not have a "path" */
		if (m->path_size!=0 && m->path_size!=65535)
			continue;

		int app_domain = strncmp(m->domain,"AppDomain",9)==0;

		if (type==LIST_SYSTEM_DOMAINS && !app_domain) {
			domains[domains_count++] = m->domain;
		}
		else if (type==LIST_APP_DOMAINS && app_domain) {
			/* "+10" skips the "AppDomain-" prefix */
			domains[domains_count++] = m->domain+10;
		}
	}

	qsort(domains, domains_count, sizeof(char*), cmpstringp);
	for (i = 0; i < domains_count ; ++i) {
		printf("%s\n", domains[i]);
	}
	free(domains);
}

/* Utility function to by used with "qosrt", comparing mbdb records */
static int
compare_mbdb_records(const void *p1, const void *p2)
{
	const mbdb_record_t* m1 = *(mbdb_record_t**)p1;
	const mbdb_record_t* m2 = *(mbdb_record_t**)p2;

	if (m1->time3 > m2->time3)
		return 1;
	if (m1->time3 < m2->time3)
		return -1;
	return strcmp(m1->path, m2->path);
}

/* Prints a sorted,user-friendly,parsable list of the images in the Cameral roll.

   For each image, the following are printed:
     SHA1 (to easy find the file on disk),
     Path (e.g. "/Media/DCIM/100APPLE/IMG_00001.JPG")
     image basename (e.g. "IMG_00001.JPG")
     image type (e.g. "JPG" or "PNG")
     image file creation date + time (e.g. "2013/10/27 13:30:59")
        note: FILE creation is based on MBDB info, not on EXIF data.
*/
void list_camera_roll(backup_t* backup)
{
	int i;
	int file_counts = backup_get_num_files(backup);
	if (file_counts<=0)
		return;

	/* There can't be more images than records in MBDB, so this is safe,
	   (but not memory efficient)*/
	size_t image_count=0;
	const mbdb_record_t **images = calloc(file_counts,sizeof(mbdb_record_t*));
	if (images==NULL)
		err(1,"calloc(images) failed");

	for (i = 0 ; i < file_counts ; ++i) {
		backup_file_t* file = backup_get_file_by_index(backup, i);

		const mbdb_record_t *m = file->mbdb_record;

		/* Skip non-files,
		   other domains,
		   and files not under "Media/DCIM" */
		if (!IS_MODE_FILE(m->mode)
		    ||
		    (strcmp(m->domain,"CameraRollDomain")!=0)
		    ||
		    (strncmp(m->path,"Media/DCIM/",11)!=0))
		    continue;

		/* Check extension.
		   TODO: is there a more reliable way to verify file type,
		         without checking the actual file data?  */
		const char* extension = rindex(m->path,'.');
		if (extension != NULL)
			++extension;

		if (extension==NULL
		    ||
		    ( strncmp(extension,"JPG",3)!=0
		      &&
		      strncmp(extension,"PNG",3)!=0 )
		   )
			continue;

		/* Add MBDB record of image to the list */
		images[image_count++] = m;
	}


	/* Sort the images, based on CTIME, then filename */
	qsort(images, image_count, sizeof(mbdb_record_t*), compare_mbdb_records);

	/* Print image information */
	for (i=0; i<image_count; ++i) {
		const mbdb_record_t *m = images[i];

		const char* extension = rindex(m->path,'.');
		if (extension != NULL)
			++extension;

		/* List file information */
		char sha1[20];
		sha1_filename(m->domain, m->path, sha1);

		const char *basename = rindex(m->path,'/');
		if (basename!=NULL)
			basename++;
		else
			basename = m->path;


		hexdump_buffer(sha1,20);
		printf(" %s %s %s", m->path, basename, extension);

		time_t t = (time_t)m->time3;
		/* TODO: gmtime or localtime? or somehow extract timezone info from the image itself? */
		struct tm* ctime = gmtime( &t ) ;
		printf(" %04d/%02d/%02d %02d:%02d:%02d",
				ctime->tm_year+1900,ctime->tm_mon+1,ctime->tm_mday,
				ctime->tm_hour,ctime->tm_min,ctime->tm_sec);

		printf("\n");
	}
}


int main(int argc, char* argv[])
{
	parse_command_line(argc,argv);

	backup_t* backup = backup_open(backup_parent_directory,
					udid);
	if (backup==NULL)
		errx(1,"error: failed to open iDevice Backup (backup_open() failed without further information)");

	switch (command)
	{
	case CMD_LIST_FILES:
		list_backup_files(backup);
		break;

	case CMD_DUMP_MBDB:
		dump_mbdb(backup);
		break;

	case CMD_LIST_DOMAINS:
		list_domains(backup,LIST_SYSTEM_DOMAINS);
		break;

	case CMD_LIST_APPS:
		list_domains(backup,LIST_APP_DOMAINS);
		break;

	case CMD_LIST_CAMERA_ROLL:
		list_camera_roll(backup);
		break;
	}

	backup_free(backup);
	return 0;
}
