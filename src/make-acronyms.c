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

MYSQL *conn;
char *output_filename = "acronyms.tex";
char *languages_priority[] = {"Portuguese", "English", NULL};
char *document_language = "Portuguese";
int fields_priority[] = {2, 7, 4, 6, 8, 10, 13, 16, 17, 0};
FILE *output;



void show_error(char *msg)
{
	fprintf(stderr, "\nError %u: %s\n", mysql_errno(conn), mysql_error(conn));
	exit(1);
}

int check_acronym_language(Acronym *acro)
{
	for (int i = 0; i < (sizeof languages_priority / sizeof (char *)); i++) {
		if (languages_priority[i] == NULL) {
			return 1;
		}
		
		if (strcasecmp(acro->language, languages_priority[i]) == 0) {
			return 1;
		}
	}

	return 0;
}

int check_acronym_type(Acronym *acro)
{
	for (int i = 0; i < sizeof(fields_priority); i++) {
		if (fields_priority[i] == 0) {
			return 1;
		}
		
		if (acro->type == fields_priority[i]) {
			return 1;
		}
	}

	return 0;
}


int compare_acronym(Acronym *acro1, Acronym *acro2)
{
	int result = 0;

	result = strcmp(acro1->name, acro2->name);
	if (result != 0) {
		return result;
	}

	if (acro1->type != acro2->type) {
		for (int i = 0; i < sizeof(fields_priority); i++) {
			if (acro1->type == fields_priority[i]) {
				return 1;
			}
			if (acro2->type == fields_priority[i]) {
				return -1;
			}
		}
	} else {
		for (int i = 0; i < (sizeof languages_priority / sizeof (char *)); i++) {
			if (strcasecmp(acro1->language, languages_priority[i]) == 0) {
				return 1;	
			}
			if (strcasecmp(acro2->language, languages_priority[i]) == 0) {
				return -1;	
			}
		}
	}

	return 0;
}

void print_acronym(Acronym *acro)
{
	if (strlen(acro->name) == 0) {
		return;
	}
	
	if (strcasecmp(acro->language, document_language) == 0) {
		fprintf(output, "\n\t\\acro{%s}{%s}", acro->name, acro->expansion);
	} else {
		fprintf(output, "\n\t\\acro{%s}{\\foreign{%s}}", acro->name, acro->expansion);
	}
}


// http://bugs.mysql.com/bug.php?id=5254
int main(int argc, char **argv)
{
	MYSQL_STMT *stmt;
	MYSQL_BIND result[4];
	
	const char *hostname = "ironiacorp.com";
	const char *database = "acronyms";
	const char *username = "acronyms-read";
	const char *password = "j4yEctYX9GFRzAmu";
	const char *sql = "select acronym, language, field_id, expansion from acronyms order by acronym, field_id";

	Acronym currAcro;
	Acronym outputAcro;

	fprintf(stdout, "\nAllowed languages: ");
	for (int i = 0; i < (sizeof languages_priority / sizeof (char *)); i++) {
		if (languages_priority[i] == NULL) {
			fprintf(stdout, "any language");
		} else {
			fprintf(stdout, "%s", languages_priority[i]);
		}
		if (i != (sizeof languages_priority / sizeof (char *)) - 1) {
			fprintf(stdout, ", ");
		}	
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "\nInitializing required libraries...");
	fprintf(stdout, "\nMySQL client version %s\n", mysql_get_client_info());
	conn = mysql_init(NULL);
	if (conn == NULL) {
		show_error("Could not load database driver");
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
		show_error("Could not connect to database server");
	}

	fprintf(stdout, "Ok");

	fprintf(stdout, "\nSetting MySQL to use UTF-8...");
	if (mysql_query(conn, "set names utf8") != 0 || mysql_query(conn, "set character set utf8") != 0) {
		show_error("Could not set charset to UTF-8");
	}
	fprintf(stdout, "Ok");


	fprintf(stdout, "\nReading acronyms...");
	stmt = mysql_stmt_init(conn);
	if (stmt == NULL) {
		show_error("Could not create prepared statement");
	}

	if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
		show_error("Could not configure prepared statement");
	}

	/*
	if (mysql_stmt_bind_param(stmt, param) != 0) {
		fprintf(stderr, "\nError %u when binding parameters to statement: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	*/

	if (mysql_stmt_execute(stmt) != 0) {
		show_error("Could not execute the prepared statement");
	}


	memset(result, 0, sizeof(result));
	memset(&currAcro, 0, sizeof(currAcro));
	memset(&outputAcro, 0, sizeof(outputAcro));
	result[0].buffer_type = MYSQL_TYPE_VAR_STRING; // acronym's name
	result[0].buffer = (void *) &currAcro.name;
	result[0].buffer_length = sizeof(currAcro.name);

	result[1].buffer_type = MYSQL_TYPE_VAR_STRING; // language
	result[1].buffer = (void *) &currAcro.language;
	result[1].buffer_length = sizeof(currAcro.language);
	
	result[2].buffer_type = MYSQL_TYPE_LONG;   // type
	result[2].buffer = (void *) &currAcro.type;
	result[2].buffer_length = sizeof(currAcro.type);
	
	result[3].buffer_type = MYSQL_TYPE_VAR_STRING; // expansion
	result[3].buffer = (void *) &currAcro.expansion;
	result[3].buffer_length = sizeof(currAcro.expansion);

	if (mysql_stmt_bind_result(stmt, result) != 0) {
		show_error("Could not bind result buffer to prepared statement");
	}


	
	if (mysql_stmt_store_result(stmt) != 0) {
		show_error("Could not retrieve results of the prepared statement");
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

	fprintf(stdout, "\nWriting acronyms into file...");
	fprintf(output, "\\begin{acronym}[ABCDEFGHIJK]");
	while (mysql_stmt_fetch(stmt) == 0) {
		if (check_acronym_language(&currAcro) == 0) {
			continue;
		}
		if (check_acronym_type(&currAcro) == 0) {
			continue;
		}

		if (strcmp(currAcro.name, outputAcro.name) == 0) {
			if (compare_acronym(&currAcro, &outputAcro) > 0) {
				memcpy(&outputAcro, &currAcro, sizeof(currAcro));
			}
		} else {
			print_acronym(&outputAcro);
			memcpy(&outputAcro, &currAcro, sizeof(currAcro));
		}
	}
	print_acronym(&outputAcro);
	fprintf(output, "\n\\end{acronym}");
	fclose(output);
	fprintf(stdout, "Ok");
	

	fprintf(stdout, "\nReleasing allocated resources...");
	mysql_stmt_free_result(stmt);
	mysql_stmt_close(stmt);
	mysql_close(conn);
	fprintf(stdout, "Ok");

	mysql_server_end();
	fprintf(stdout, "\n");
	return 0;
}

