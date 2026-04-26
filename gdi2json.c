#include <argp.h>
#include <cjson/cJSON.h>
#include <error.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct GDITrack {
	uint32_t trackID;    // 轨号
	uint32_t startLBA;   // 起始LBA
	uint32_t trackType;  // 轨类型
	uint32_t sectorSize; // 扇区大小
	char* fileName;	     // 文件名
	uint32_t offset;     // 偏移
} GDITrack;

typedef struct GDI {
	uint32_t trackCount;
	GDITrack** trackList;
} GDI;

void dump_from_gdi(FILE* restrict gdiFile, GDI* restrict gdi) // 读取gdi文件，写入结构体
{
	fscanf(gdiFile, "%u", &gdi->trackCount); // 首行，轨数

	gdi->trackList = (GDITrack**)malloc(gdi->trackCount * sizeof(GDITrack*));
	if (not gdi->trackList) {
		error(ENOMEM, ENOMEM, "%s：为gdi->trackList malloc失败", __func__);
	}

	char fileNameTemp[1024], *p_fileNameTemp;
	int currentChar;
	for (uint32_t i = 0; i < gdi->trackCount; i += 1) {
		gdi->trackList[i] = (GDITrack*)malloc(sizeof(GDITrack));
		if (not gdi->trackList[i]) {
			error(ENOMEM, ENOMEM, "%s：为gdi->trackList[%u] malloc失败", __func__, i);
		}

		// 读取前4参数
		fscanf(gdiFile, "%u%u%u%u",
		    &gdi->trackList[i]->trackID,
		    &gdi->trackList[i]->startLBA,
		    &gdi->trackList[i]->trackType,
		    &gdi->trackList[i]->sectorSize);

		// 读取文件名
		p_fileNameTemp = fileNameTemp;
		memset(fileNameTemp, '\0', 1024 * sizeof(char));

		// fscanf(gdiFile, " %c", (char*)&currentChar); // 利用fscanf行为跳过' '、'\t'、'\n'，试读一个字
		while ((currentChar = fgetc(gdiFile)) != EOF and(currentChar == ' ' or currentChar == '\t' or currentChar == '\n')); // 孩子们，还是得老老实实写while
		// ungetc(currentChar, gdiFile);
		// printf("%c\t%lu\n", currentChar, ftell(gdiFile));

		if (currentChar == '"') { // 发现双引号，则读取到另一个双引号前
			while (true) {
				currentChar = fgetc(gdiFile);
				if (currentChar == '"') {
					break;
				}
				*p_fileNameTemp = currentChar;
				p_fileNameTemp += 1;
			}
		} else { // 否则读取到上述几个空格为止
			while (not(currentChar == ' ' or currentChar == '\t' or currentChar == '\n')) {
				*p_fileNameTemp = currentChar;
				p_fileNameTemp += 1;
				currentChar = fgetc(gdiFile);
			}
		}

		// 拷贝临时数组中的字符串
		gdi->trackList[i]->fileName = (char*)malloc((strlen(fileNameTemp) + 1) * sizeof(char));
		if (not gdi->trackList[i]->fileName) {
			error(ENOMEM, ENOMEM, "%s：为gdi->trackList[%u]->fileName malloc失败", __func__, i);
		}
		strcpy(gdi->trackList[i]->fileName, fileNameTemp); // strcpy()有自动追加'\0'的行为

		fscanf(gdiFile, "%u", &gdi->trackList[i]->offset); // 读取最后的偏移参数
	}
}

void write_to_json(FILE* jsonFile, const GDI* gdi)
{
	cJSON* root = cJSON_CreateObject();
	if (not root) {
		error(EPERM, EPERM, "%s：无法创建cJSON对象", __func__);
	}

	cJSON_AddNumberToObject(root, "trackCount", gdi->trackCount);

	cJSON* trackList = cJSON_AddArrayToObject(root, "trackList");
	for (uint32_t i = 0; i < gdi->trackCount; i += 1) {
		cJSON* track = cJSON_CreateObject();
		cJSON_AddNumberToObject(track, "trackID", gdi->trackList[i]->trackID);
		cJSON_AddNumberToObject(track, "startLBA", gdi->trackList[i]->startLBA);
		cJSON_AddNumberToObject(track, "trackType", gdi->trackList[i]->trackType);
		cJSON_AddNumberToObject(track, "sectorSize", gdi->trackList[i]->sectorSize);
		cJSON_AddStringToObject(track, "fileName", gdi->trackList[i]->fileName);
		cJSON_AddNumberToObject(track, "offset", gdi->trackList[i]->offset);

		cJSON_AddItemToArray(trackList, track);
	}

	char* jsonFile_content = cJSON_Print(root);
	fputs(jsonFile_content, jsonFile);

	if (jsonFile_content) {
		free(jsonFile_content);
	}
	if (root) {
		cJSON_Delete(root);
	}
}

void GDI_Clear(GDI* restrict gdi)
{
	for (uint32_t i = 0; i < gdi->trackCount; i += 1) {
		if (gdi->trackList[i]) {
			if (gdi->trackList[i]->fileName) {
				free(gdi->trackList[i]->fileName);
				gdi->trackList[i]->fileName = NULL;
			}
			free(gdi->trackList[i]);
			gdi->trackList[i] = NULL;
		}
	}
}

/* ************************************************ */

typedef struct Args {
	char* gdiFilePath;
	char* jsonFilePath;
	bool autoJsonFilePath;
} Args;

static const struct argp_option options[] = {
	{ "gdiFilePath", 'g', "FILE", 0, "GDI文件路径" },
	{ "jsonFilePath", 'j', "FILE", 0, "json文件路径" },
	{ 0 }
};

static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
	Args* args = state->input;

	switch (key) {
	case 'g': {
		args->gdiFilePath = arg;
		break;
	}
	case 'j': {
		args->jsonFilePath = arg;
		args->autoJsonFilePath = false;
		break;
	}
	}
	return 0;
}

static struct argp argp = { options, parse_opt, NULL, "一款将Sega DC光碟镜像索引.gdi文件改编为json格式的工具。" };

int main(int argc, char** argv)
{
	Args args = {
		.gdiFilePath = NULL,
		.jsonFilePath = NULL,
		.autoJsonFilePath = true
	};

	if (argc == 1) {
		argp_help(&argp, stdout, ARGP_HELP_USAGE | ARGP_HELP_LONG, NULL);
		error(EINVAL, EINVAL, "程序未接收到任何参数……");
	}

	argp_parse(&argp, argc, argv, 0, 0, &args);

	if (args.gdiFilePath == NULL) {
		error(EINVAL, EINVAL, "%s：-g/--gdiFilePath参数必须传入", __func__);
	}

	if (args.autoJsonFilePath) {
		uint32_t len_jsonFilePath = strlen(args.gdiFilePath) + 1 + 1;
		// printf("%u\n", len_jsonFilePath);
		args.jsonFilePath = (char*)malloc(len_jsonFilePath * sizeof(char));
		if (not args.jsonFilePath) {
			error(ENOMEM, ENOMEM, "%s：为args.jsonFilePath malloc失败", __func__);
		}
		memset(args.jsonFilePath, '\0', len_jsonFilePath * sizeof(char));
		strcpy(args.jsonFilePath, args.gdiFilePath);

		char* jsonFilePathExtend = strrchr(args.jsonFilePath, '.');
		strcpy(jsonFilePathExtend, ".json");

		fprintf(stderr, "因未传入json文件路径，自动推导json文件路径为：“%s”\n", args.jsonFilePath);
	}

	FILE *gdiFile = fopen(args.gdiFilePath, "rt"), *jsonFile = fopen(args.jsonFilePath, "wt");
	if (not gdiFile) {
		error(ENOENT, ENOENT, "%s：未能打开文件“%s”", __func__, args.gdiFilePath);
	}
	if (not jsonFile) {
		error(EINVAL, EINVAL, "%s：未能打开文件“%s”", __func__, args.jsonFilePath);
	}

	GDI gdi;
	dump_from_gdi(gdiFile, &gdi);
	write_to_json(jsonFile, &gdi);

	GDI_Clear(&gdi);

	return 0;
}
