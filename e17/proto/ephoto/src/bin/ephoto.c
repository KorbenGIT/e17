#include "ephoto.h"

int main(int argc, char **argv)
{
	char *album, *name, *description, *path, input[2];
	int i;
	sqlite3 *db;

	/*Check to make sure EWL is accessible*/
        if (!ewl_init(&argc, argv))
        {
                printf("Ewl is not usable, please check your installation!\n");
                return 1;
        }

	for(i = 0; i < argc; i++)
	{
		if(!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
		{
			printf("Ephoto Version %s Help Page\n", VERSION);
			printf("Long Commands: \n"
			       " --help			-	This Screen\n"
			       " --add-album %%s %%s	-	Adds Album\n"
			       "			 	arg1 = name of album\n"
			       "				arg2 = description of album\n"
			       " --add-image %%s %%s %%s	-	Adds Image\n"
			       "				arg1 = name of album to add image to\n"
			       "				arg2 = descriptive name for image\n"
			       "				arg3 = path to the image\n"
			       " --remove-album	%%s	-	Removes Album\n"
			       "				arg1 = name of album\n"
			       " --remove-image	%%s %%s	-	Removes Image\n"
			       "				arg1 = name of album to remove image from\n"
			       "				arg2 = path of the image to be removed\n");
			printf("Short Commands: \n"
			       " -h			-	This Screen\n"
			       " -a	%%s %%s		-	Adds Album\n"
			       " -i	%%s %%s %%s	-	Adds Image\n"
			       " -ra	%%s		-	Removes Album\n"
			       " -ri	%%s %%s		-	Removes Image\n");
			return 0;
		}
		if(!strcmp(argv[i], "--add-album") || !strcmp(argv[i], "-a"))
		{
			i++;
			if(argv[i]) name = argv[i];
			else
			{
				printf("Please specify a name for the album\n");
				return 1;
			}

			i++;
			if(argv[i]) description = argv[i];
			else 
			{
				printf("Please specify a description for the album\n");
				return 1;
			}

			printf("Are you sure you want to create an album with "
			       "the name %s and the description %s? ", 
			       name, description);
			scanf("%c", &input);
			if(!strncasecmp(input, "y", 1))
			{
				db = ephoto_db_init();
				ephoto_db_add_album(db, name, description);
				ephoto_db_close(db);
				printf("Album was created\n");
			}
			else printf("Album was not created\n");

			return 0;
		}
		if(!strcmp(argv[i], "--add-image") || !strcmp(argv[i], "-i"))
		{
			i++;
			if(argv[i]) album = argv[i];
			else
			{
				printf("Please specify the album you wish to add to\n");
				return 1;
			}

			i++;
			if(argv[i]) name = argv[i];
			else 
			{
				printf("Please specify a descriptive name for the image\n");
				return 1;
			}

			i++;
			if(argv[i]) path = argv[i];
			else 
			{
				printf("Please specify the path to the image\n");
				return 1;
			}

			printf("Are you sure you want to add an image "
			       "to album %s with a name %s and path %s? ", 
			       album, name, path);
			scanf("%c", &input);
			if(!strncasecmp(input, "y", 1))
			{
				db = ephoto_db_init();
				ephoto_db_add_image(db, album, name, path);
				ephoto_db_close(db);
				printf("Image was added\n");
			}
			else printf("Image was not added\n");

			return 0;
		}
		if(!strcmp(argv[i], "--remove-album") || !strcmp(argv[i], "-ra"))
		{
			i++;
			if(argv[i]) name = argv[i];
			else
			{
				printf("Please specify the name of the album\n");
				return 1;
			}

			printf("Are you sure you want to remove the album %s? ", 
			       name);
			scanf("%c", &input);
			if(!strncasecmp(input, "y", 1))
			{
				db = ephoto_db_init();
				ephoto_db_delete_album(db, name);
				ephoto_db_close(db);
				printf("Album was removed\n");
			}
			else printf("Album was not removed\n");

			return 0;
		}
		if(!strcmp(argv[i], "--remove-image") || !strcmp(argv[i], "-ri"))
		{
			i++;
			if(argv[i]) album = argv[i];
			else
			{
				printf("Please specify the name of the album "
				       "the image belongs to.\n");
				return 1;
			}

			i++;
			if(argv[i]) path = argv[i];
			else 
			{
				printf("Please specify the path of the image\n");
				return 1;
			}

			printf("Are you sure you want to remove the image %s "
			       "from the album %s? ", 
			       path, album);
			scanf("%c", &input);
			if(!strncasecmp(input, "y", 1))
			{
				db = ephoto_db_init();
				ephoto_db_delete_image(db, album, path);
				ephoto_db_close(db);
				printf("Image was removed\n");
			}
			else printf("Image was not removed\n");

			return 0;
		}
	}

	/* NLS */
#ifdef ENABLE_NLS
	setlocale(LC_MESSAGES, "");
	bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(PACKAGE, "UTF-8");
	textdomain(PACKAGE);
#endif
	/*Use our own custom theme*/
//	ewl_theme_theme_set(PACKAGE_DATA_DIR "/themes/ephoto.edj");
	
	/*Start the GUI*/
	create_main_gui();

	return 0;
}
