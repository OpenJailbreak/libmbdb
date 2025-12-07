/**
  * libmbdb-1.0 - backup_file.h
  * Copyright (C) 2013 Crippy-Dev Team
  * Copyright (C) 2010-2013 Joshua Hill
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

#include <libmbdb-1.0/mbdb.h>
#include <libcrippy-1.0/debug.h>

mbdb_t* mbdb_create() {
	mbdb_t* mbdb = NULL;

	mbdb = (mbdb_t*) malloc(sizeof(mbdb_t));
	if(mbdb == NULL) {
		return NULL;
	}
	memset(mbdb, '\0', sizeof(mbdb_t));
	return mbdb;
}

mbdb_t* mbdb_parse(unsigned char* data, unsigned int size) {
	int i = 0;
	unsigned int count = 0;
	unsigned int offset = 0;

	mbdb_t* mbdb = NULL;
	mbdb_header_t* header = NULL;
	mbdb_record_t* record = NULL;

	mbdb = mbdb_create();
	if(mbdb == NULL) {
		error("Unable to create mbdb\n");
		return NULL;
	}

	header = (mbdb_header_t*) data;
	if(strncmp(header->magic, MBDB_MAGIC, 6) != 0) {
		error("Unable to identify this filetype\n");
		return NULL;
	}

	// Copy in our header data
	mbdb->header = (mbdb_header_t*)malloc(sizeof(mbdb_header_t));
	if(mbdb->header == NULL) {
		error("Allocation error\n");
		return NULL;
	}
	memset(mbdb->header, '\0', sizeof(mbdb_header_t));
	memcpy(mbdb->header, &data[offset], sizeof(mbdb_header_t));
	offset += sizeof(mbdb_header_t);

	mbdb->data = (unsigned char*)malloc(size);
	if (mbdb->data == NULL) {
		error("Allocation Error!!\n");
		return NULL;
	}
	memcpy(mbdb->data, data, size);
	mbdb->size = size;

	size_t records_capacity = (mbdb->size / 64) + 1;
	mbdb->records = (mbdb_record_t**)malloc(records_capacity * sizeof(*mbdb->records));
	if(mbdb->records == NULL) {
		error("Unable to allocate record table\n");
		return NULL;
	}
	mbdb->num_records = 0;

	while (offset < mbdb->size) {
		if (mbdb->num_records >= (int)records_capacity) {
			size_t new_capacity = records_capacity * 2;
			mbdb_record_t** new_records = (mbdb_record_t**)realloc(
				mbdb->records, new_capacity * sizeof(*mbdb->records));
			if (!new_records) {
				error("Unable to grow record table\n");
				break;
			}
			mbdb->records = new_records;
			records_capacity = new_capacity;
		}
		mbdb_record_t* rec = mbdb_record_parse(&(mbdb->data)[offset]);
		if (!rec) {
			error("Unable to parse record at offset 0x%x!\n", offset);
			break;
		}
		mbdb->records[mbdb->num_records++] = rec;
		offset += rec->this_size;
	}

	return mbdb;
}

mbdb_t* mbdb_open(unsigned char* file) {
	int err = 0;
	unsigned int size = 0;
	unsigned char* data = NULL;

	mbdb_t* mbdb = NULL;

	err = file_read(file, &data, &size);
	if(err < 0) {
		error("Unable to read mbdb file\n");
		return NULL;
	}

	mbdb = mbdb_parse(data, size);
	if(mbdb == NULL) {
		error("Unable to parse mbdb file\n");
		return NULL;
	}

	free(data);
	return mbdb;
}

mbdb_record_t* mbdb_get_record(mbdb_t* mbdb, unsigned int index)
{
	if (!mbdb || !mbdb->records) {
		return NULL;
	}
	if (index >= (unsigned int)mbdb->num_records) {
		return NULL;
	}
	return mbdb->records[index];
}

void mbdb_free(mbdb_t* mbdb) {
	if(mbdb) {
		if(mbdb->header) {
			free(mbdb->header);
			mbdb->header = NULL;
		}
		if(mbdb->records) {
			int i;
			for (i = 0; i < mbdb->num_records; i++) {
				mbdb_record_free(mbdb->records[i]);
			}
			free(mbdb->records);
		}
		if(mbdb->data) {
			free(mbdb->data);
		}
		free(mbdb);
	}
}
