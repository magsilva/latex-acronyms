#include <stdio.h>
#include <my_global.h>
#include <mysql.h>
#include <string.h>

int main(int argc, char **argv)
{
	FILE *output;

	MYSQL *conn;
	MYSQL_RES *result;
	MYSQL_ROW row;

	char *document_language = "Portuguese";
	char *allowed_languages[] = {"Portuguese", "English", NULL};

	fprintf(stdout, "\nDocument language: %s", document_language);

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
	fprintf(stderr, "\nMySQL client version %s\n", mysql_get_client_info());
	fprintf(stdout, "Ok");


	fprintf(stdout, "\nSetting up database connection...");
	conn = mysql_init(NULL);
	if (conn == NULL) {
		fprintf(stderr, "%u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	fprintf(stderr, "\nEnabling compression of data...");
	mysql_options(conn, MYSQL_OPT_COMPRESS, NULL);

	fprintf(stderr, "\nSetting timeouts...");
	mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, "240");
	mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, "40");
	mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, "40");

	fprintf(stderr, "\nEnabling reconnection if connection accidently broken or timed out");
	mysql_options(conn, MYSQL_OPT_RECONNECT, "true");

	if (mysql_real_connect(conn, "143.107.183.152", "acronyms-read", "j4yEctYX9GFRzAmu", "acronyms", 0, NULL, 0) == NULL) {
		fprintf(stderr, "\nError %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	fprintf(stdout, "Ok");


	fprintf(stdout, "\nReading acronyms...");
	if (mysql_query(conn, "select acronym, language, expansion from acronyms order by acronym")) {
		printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}

	fprintf(stderr, "\nReading rowsets...");
	result = mysql_store_result(conn);
	fprintf(stdout, "Ok");

	fprintf(stdout, "\nCreating acronyms file...");
	output = fopen("acronyms.tex", "w");
	if (output == NULL) {
		fprintf(stderr, "Error creating the acronyms file");
		exit(1);
	}
	fprintf(stdout, "Ok");

	fprintf(stdout, "\nWriting acronyms into file");
	fprintf(output, "\\begin{acronym}[ABCDEFGHIJK]");
	while ((row = mysql_fetch_row(result))) {
		int found = 0;
		for (int i = 0; found == 0 && i < (sizeof allowed_languages / sizeof (char *)); i++) {
			if (row[1] == NULL) {
				if (allowed_languages[i] == NULL) {
					fprintf(output, "\n\t\\acro{%s}{%s}", row[0], row[2]);
					found = 1;		
				}
			} else {
				if (allowed_languages[i] == NULL || strcmp(row[1], allowed_languages[i]) == 0) {
					if (strcmp(row[1], document_language) == 0) {
						fprintf(output, "\n\t\\acro{%s}{%s}", row[0], row[2]);
					} else {
						fprintf(output, "\n\t\\acro{%s}{\\foreign{%s}}", row[0], row[2]);
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
	mysql_free_result(result);
	mysql_close(conn);
	fprintf(stdout, "Ok");

	fprintf(stdout, "\n");
	return 0;
}

