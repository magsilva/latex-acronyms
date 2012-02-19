#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <mysql.h>

typedef struct {
	char name[32];
	int  type;
	char language[30];
	char expansion[255];
} Acronym; 


// http://bugs.mysql.com/bug.php?id=5254
int main(int argc, char **argv)
{
	MYSQL *conn;
	MYSQL_STMT *stmt;
	MYSQL_BIND result[4];
	
	char *hostname = "ironiacorp.com";
	char *database = "acronyms";
	char *username = "acronyms-read";
	char *password = "j4yEctYX9GFRzAmu";
	char *sql = "select acronym, language, field_id, expansion from acronyms order by acronym, field_id";

	Acronym currAcro;
	Acronym prevAcro;

	FILE *output;

	char *output_filename = "acronyms.tex";
	char *document_language = "Portuguese";
	char *allowed_languages[] = {"Portuguese", "English", NULL};
	int fields_priority[] = {2, 7, 4, 6, 8, 10, 13, 16, 17};

	fprintf(stdout, "\nAllowed languages: ");
	for (int i = 0; i < (sizeof allowed_languages / sizeof (char *)); i++) {
		if (allowed_languages[i] == NULL) {
			fprintf(stdout, "any language");
		} else {
			fprintf(stdout, "%s", allowed_languages[i]);
		}
		if (i != (sizeof allowed_languages / sizeof (char *)) - 1) {
			fprintf(stdout, ", ");
		}	
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "\nInitializing required libraries...");
	fprintf(stdout, "\nMySQL client version %s\n", mysql_get_client_info());
	conn = mysql_init(NULL);
	if (conn == NULL) {
		fprintf(stderr, "%u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	fprintf(stdout, "Ok");

	fprintf(stdout, "\nSetting character encoding to UTF-8...");
	mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8");
	fprintf(stdout, "Ok");

	fprintf(stdout, "\nEnabling compression of data...");
	mysql_options(conn, MYSQL_OPT_COMPRESS, NULL);
	fprintf(stdout, "Ok");

	fprintf(stdout, "\nSetting timeouts...");
	mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, "240");
	mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, "40");
	mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, "40");
	fprintf(stdout, "Ok");

	fprintf(stdout, "\nEnabling reconnection if connection accidently broken or timed out...");
	mysql_options(conn, MYSQL_OPT_RECONNECT, "true");
	fprintf(stdout, "Ok");

	fprintf(stdout, "\nConnecting to database %s at %s...", database, hostname);
	if (mysql_real_connect(conn, hostname, username, password, database, 0, NULL, 0) == NULL) {
		fprintf(stderr, "\nError %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	fprintf(stdout, "Ok");

	fprintf(stdout, "\nSetting MySQL to use UTF-8...");
	if (mysql_query(conn, "set names utf8") != 0 || mysql_query(conn, "set character set utf8") != 0) {
		fprintf(stderr, "\nError setting charset to UTF-8 (%u): %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	fprintf(stdout, "Ok");


	fprintf(stdout, "\nReading acronyms...");
	stmt = mysql_stmt_init(conn);
	if (stmt == NULL) {
		fprintf(stderr, "\nError %u when creating statement: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	if (mysql_stmt_prepare(stmt, sql, strlen(sql))) {
		fprintf(stderr, "\nError %u when preparing statement: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	memset(result, 0, sizeof(result));
	memset(&currAcro, 0, sizeof(currAcro));
	memset(&prevAcro, 0, sizeof(prevAcro));
	result[0].buffer_type = MYSQL_TYPE_STRING; // acronym's name
	result[0].buffer = (void *) &currAcro.name;
	result[1].buffer_type = MYSQL_TYPE_STRING; // language
	result[1].buffer = (void *) &currAcro.language;
	result[2].buffer_type = MYSQL_TYPE_LONG;   // type
	result[2].buffer = (void *) &currAcro.type;
	result[3].buffer_type = MYSQL_TYPE_STRING; // expansion
	result[3].buffer = (void *) &currAcro.expansion;

	/*
	if (mysql_stmt_bind_param(stmt, param) != 0) {
		fprintf(stderr, "\nError %u when binding parameters to statement: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	*/

	if (mysql_stmt_bind_result(stmt, result) != 0) {
		fprintf(stderr, "\nError %u when binding result buffer to statement: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	if (mysql_stmt_execute(stmt) != 0) {
		fprintf(stderr, "\nError %u when executing statement: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	if (mysql_stmt_store_result(stmt) != 0) {
		fprintf(stderr, "\nError %u when retrieving results of statement: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	fprintf(stdout, "Ok");

	fprintf(stdout, "\nCreating acronyms file...");
	output = fopen(output_filename, "w");
	if (output == NULL) {
		fprintf(stderr, "\nError creating the acronyms file");
		exit(1);
	}
	fprintf(stdout, "Ok");

	fprintf(stdout, "\nWriting acronyms into file");
	fprintf(output, "\\begin{acronym}[ABCDEFGHIJK]");
	while (mysql_stmt_fetch(stmt) == 0) {
		int found = 0;
		fprintf(stdout, acro.name);
		for (int i = 0; found == 0 && i < (sizeof allowed_languages / sizeof (char *)); i++) {
			if (strcasecmp(currAcro.language, allowed_languages[i]) == 0) {
				if (strcasecmp(currAcro.language, document_language) == 0) {
					if (strcmp(currAcro.name, prevAcro.name) == 0) {
						fprintf(output, "\n\t\\acro{%s}{%s}", currAcro.name, currAcro.expansion);
					} else {
						fprintf(output, "\n\t\\acro{%s}{\\foreign{%s}}", currAcro.name, currAcro.expansion);
					}
					found = 1;
				}
			}
		}
		if (found) {
			fprintf(stdout, ".");
		} else {
			fprintf(stdout, "X");
		}
	}
	fprintf(output, "\n\\end{acronym}");
	fclose(output);
	fprintf(stdout, "Ok");
	

	fprintf(stdout, "\nReleasing allocated resources...");
	mysql_stmt_free_result(stmt);
	mysql_close(conn);
	fprintf(stdout, "Ok");

	fprintf(stdout, "\n");
	return 0;
}

