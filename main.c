#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#if defined WIN32
    #include <sys/types.h>
#endif

typedef struct File
{
  int name_size;
  char* name;
  int offset;
  int size;
}File;

typedef struct Dir
{
  int name_size;
  char* name;
  int nb_files;
  File* files;
}Dir;


typedef struct Head
{
  Dir* dirs;
  int root;
}Head;

int nombre_dossier(char chaine[])
{
    int i = 0;
    int nb = 0;

    while(chaine[i] != '\0')
    {
        if(chaine[i] == '/')
        {
            nb++;
        }
        i++;
    }
    return nb;
}

void make_dirs(char* out)
{
    char temp[250] = {0};
    int i = 0;
    int nb = 0;
    char* dir = NULL;
    char path[250] = {0};
    DIR* directory = NULL;

    sprintf(temp, "%s", out);

    dir = strtok(temp, "/");

    if(dir != NULL)
    {

        nb = nombre_dossier(out);

        for(i = 0; i < nb; i++)
        {
            strcat(path, dir);
            strcat(path, "/");
            directory = opendir(path);
            if(!directory)
            {
                mkdir(path);
                closedir(directory);
            }
            else
            {
                closedir(directory);
            }

            dir = strtok(NULL, "/");
        }
    }
}

void get_files(FILE* in, Dir* d);

void get_dir(FILE* in, Head* h)
{
  fread(&h->root, sizeof(char), 1, in);
  h->dirs = calloc(h->root, sizeof(Dir));
  for(int i = 0; i < h->root; i++)
  {
    int tmp = 0;
    fread(&h->dirs[i].name_size, sizeof(char), 1, in);
    h->dirs[i].name = calloc(h->dirs[i].name_size+1, sizeof(char));
    fread(h->dirs[i].name, sizeof(char), h->dirs[i].name_size, in);
    fread(&tmp, sizeof(char), 1, in);
    if(tmp == 0xFC)
    {
      fread(&h->dirs[i].nb_files, sizeof(char), 2, in);
    }
    else
    {
      h->dirs[i].nb_files = tmp;
    }
    printf("Name size : %d\nName : %s\nFiles : %X\n", h->dirs[i].name_size, h->dirs[i].name, h->dirs[i].nb_files);
    h->dirs[i].files = calloc(h->dirs[i].nb_files, sizeof(File));
    make_dirs(h->dirs[i].name);
    get_files(in, &h->dirs[i]);
  }

}

void dump_files(FILE* in, Head* h)
{
  unsigned char a = 0;
  long current = ftell(in);
  for(int i = 0; i < h->root; i++)
  {
    for(int j = 0; j < h->dirs[i].nb_files; j++)
    {
      char dir[250] = {0};
      sprintf(dir, "%s", h->dirs[i].name);
      strcat(dir, h->dirs[i].files[j].name);
      FILE* out = fopen(dir, "wb");
      printf("Dumping %s...", dir);
      fflush(stdout);
      fseek(in, h->dirs[i].files[j].offset+current, SEEK_SET);
      for(int k = 0; k < h->dirs[i].files[j].size; k++)
      {
        fread(&a, sizeof(char), 1, in);
        fwrite(&a, sizeof(char), 1, out);
      }
      fclose(out);
      printf("....done !\n");
    }
  }


}

void get_files(FILE* in, Dir* d)
{
  for(int i = 0; i < d->nb_files; i++)
  {
    fread(&d->files[i].name_size, sizeof(char), 1, in);
    d->files[i].name = calloc(d->files[i].name_size+1, sizeof(char));
    fread(d->files[i].name, sizeof(char), d->files[i].name_size, in);
    fread(&d->files[i].offset, sizeof(char), 4, in);
    fread(&d->files[i].size, sizeof(char), 4, in);
   // printf("Name size : 0x%.8X\nName : %s\nOffset : 0x%.8X\nSize : 0x%.8X\nI : %d/%d\n", d->files[i].name_size, d->files[i].name, d->files[i].offset, d->files[i].size, i+1, d->nb_files);
  }
}

int main(int argc, char* argv[])
{
  FILE* in = fopen("GAMEDATA.CFI", "rb");
  Head h = {NULL, 0};
  if(!in)
  {
	printf("Impossible d'ouvrir le fichier %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  get_dir(in, &h);
  dump_files(in, &h);
  for(int i = 0; i < h.root; i++)
  {
    for(int j = 0; j < h.dirs[i].nb_files; j++)
    {
      free(h.dirs[i].files[j].name);
    }
    free(h.dirs[i].files);
    free(h.dirs[i].name);
  }
  free(h.dirs);

  return EXIT_SUCCESS;
}
