/*
 * Art.cpp
 *
 *  Created on: 10.11.2012
 *      Author: Denis Zagayevskiy
 */

#include "Art.h"

const char* Art::LEVELS_PATH = "levels";

AAssetManager* Art::assetManager;
jobject Art::pngManager;
JNIEnv* Art::pmEnv;
jclass Art::pmClass;
jmethodID Art::pmOpenId;
jmethodID Art::pmCloseId;
jmethodID Art::pmGetWidthId;
jmethodID Art::pmGetHeightId;
jmethodID Art::pmGetPixelsId;

Texture** Art::texturesSources = NULL;
GLuint* Art::textures = NULL;

char** Art::shadersSources = NULL;

Texture** Art::levels = NULL;
GLfloat** Art::levelsTexCoords = NULL;
int Art::levelsCount = 0;

void Art::init(JNIEnv* env, jobject _pngManager, jobject javaAssetManager){
	LOGI("Art::init");
	free();
	pngManager = _pngManager;
	pmEnv = env;

	pmClass = env->GetObjectClass(pngManager);
	pmOpenId = env->GetMethodID(pmClass, "open", "(Ljava/lang/String;)Landroid/graphics/Bitmap;");
	pmCloseId = env->GetMethodID(pmClass, "close", "(Landroid/graphics/Bitmap;)V");
	pmGetWidthId = env->GetMethodID(pmClass, "getWidth", "(Landroid/graphics/Bitmap;)I");
	pmGetHeightId = env->GetMethodID(pmClass, "getHeight", "(Landroid/graphics/Bitmap;)I");
	pmGetPixelsId = env->GetMethodID(pmClass, "getPixels", "(Landroid/graphics/Bitmap;[I)V");

	assetManager = AAssetManager_fromJava(env, javaAssetManager);

	loadLevels();

	texturesSources = new Texture*[TEXTURES_COUNT];
	texturesSources[TEXTURE_PLAY_BUTTON] = loadPng("textures/button1.png");
	texturesSources[TEXTURE_GRAD] = loadPng("textures/tex.png");
	texturesSources[TEXTURE_YELLOW] = loadPng("textures/yellow.png");
	texturesSources[TEXTURE_PACMAN] = loadPng("textures/pacman.png");
	texturesSources[TEXTURE_MAP_0] = loadPng("textures/map.png");
	texturesSources[TEXTURE_MONSTER] = loadPng("textures/monster.png");
	texturesSources[TEXTURE_BUTTONS] = loadPng("textures/buttons.png");
	texturesSources[TEXTURE_ALL_LEVELS] = makeTextureFromLevels();

	shadersSources = new char*[SHADERS_COUNT];
	shadersSources[SHADER_VERTEX_0] = loadTextFile("shaders/shader.vrt");
	shadersSources[SHADER_FRAGMENT_0] = loadTextFile("shaders/shader.frg");
	shadersSources[SHADER_VERTEX_SHIFT] = loadTextFile("shaders/shiftShader.vrt");

}

void Art::generateTextures(){
	LOGI("Art::generateTextures");

	textures = new GLuint[TEXTURES_COUNT];
	for(int i = 0; i < TEXTURES_COUNT; ++i){
		textures[i] = texturesSources[i] ? createTexture(texturesSources[i]) : TEXTURE_NONE;
	}
}

GLuint Art::getTexture(int id){

	if(0 <= id && id < TEXTURES_COUNT && textures){
		return textures[id];
	}else{
		return TEXTURE_NONE;
	}
}

char* Art::getShaderSource(int id){
	LOGI("Art::getShaderSource(%d)", id);

	if(0 <= id && id < SHADERS_COUNT){
		return shadersSources[id];
	}else{
		return NULL;
	}
}

Texture* Art::getLevel(int number){
	return (number >= 0 && number < levelsCount) ? levels[number] : NULL;
}

GLfloat* Art::getLevelTexCoords(int number){
	LOGI("Art::getLevelTexCoords(%d)", number);
	return (number >= 0 && number < levelsCount) ? levelsTexCoords[number] : NULL;
}

void Art::free(){
	LOGI("Art::free");

	if(textures){
		delete[] textures;
	}

	if(texturesSources){
		for(int i = 0; i < TEXTURES_COUNT; ++i){
			if(texturesSources[i]){
				delete texturesSources[i];
			}
		}
		delete[] texturesSources;
	}

	if(shadersSources){
		for(int i = 0; i < SHADERS_COUNT; ++i){
			if(shadersSources[i]){
				delete[] shadersSources[i];
			}
		}
		delete[] shadersSources;
	}

	if(levels){
		for(int i = 0; i < levelsCount; ++i){
			if(levels[i]){
				delete levels[i];
			}
		}
		delete[] levels;
	}

	if(levelsTexCoords){
		for(int i = 0; i < levelsCount; ++i){
			if(levelsTexCoords[i]){
				delete levelsTexCoords[i];
			}
		}
		delete[] levelsTexCoords;
	}

}

Texture* Art::loadPng(const char* filename){
	LOGI("Art::loadPng(%s)", filename);

	Texture* result = new Texture();

	jstring name = pmEnv->NewStringUTF(filename);
	jobject png = pmEnv->CallObjectMethod(pngManager, pmOpenId, name);
	pmEnv->DeleteLocalRef(name);
	pmEnv->NewGlobalRef(png);

	jint width = pmEnv->CallIntMethod(pngManager, pmGetWidthId, png);
	jint height = pmEnv->CallIntMethod(pngManager, pmGetHeightId, png);

	jintArray array = pmEnv->NewIntArray(width * height);
	pmEnv->NewGlobalRef(array);
	pmEnv->CallVoidMethod(pngManager, pmGetPixelsId, png, array);

	jint *pixels = pmEnv->GetIntArrayElements(array, 0);

	result->pixels = argb2rgba((unsigned int*)pixels, width, height);
	result->width = width;
	result->height = height;

	pmEnv->ReleaseIntArrayElements(array, pixels, 0);
	pmEnv->CallVoidMethod(pngManager, pmCloseId, png);

	return result;
}

GLuint Art::createTexture(Texture* texture){
	LOGI("Art::createTexture");

	GLuint textureId;
	glGenTextures(1, &textureId);
	LOGI("Generated texture id = %i", textureId);

	glBindTexture(GL_TEXTURE_2D, textureId);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->pixels);

	return textureId;
}

char* Art::loadTextFile(const char* filename){
	LOGI("Art::loadTextFile(%s)", filename);

	AAsset* asset = AAssetManager_open(assetManager, filename, AASSET_MODE_BUFFER);
	long length = AAsset_getLength(asset);

	char* buffer = new char[length];
	long offset = 0;
	long readed = 0;
	while((readed = AAsset_read(asset, buffer + offset, length - offset)) > 0){
		offset += readed;
	}

	AAsset_close(asset);

	buffer[length - 1] = '\0';

	return buffer;
}

char* Art::argb2rgba(unsigned int* pixels, int w, int h){
	char* result = new char[w*h*4];

	unsigned int pixel;
	int i, j, offset;

	for(i = 0; i < h; ++i){
		offset = i*w*4;
		for(j = 0; j < w*4; j += 4){
			pixel = pixels[i*w + j / 4];
			result[offset + j] 		= (pixel >> 16) & 0xff;
			result[offset + j + 1]  = (pixel >> 8) & 0xff;
			result[offset + j + 2]  = pixel & 0xff;
			result[offset + j + 3]  = (pixel >> 24) & 0xff;
		}
	}
	return result;
}

List<char*> Art::loadFilesList(const char* path){
	LOGI("Art::loadFilesList(%s)", path);

	List<char*> result;
	AAssetDir* assetDir = AAssetManager_openDir(assetManager, path);
	const char* c;
	while(((c = AAssetDir_getNextFileName(assetDir)) != NULL)){
		char * buffer = new char[MAX_PATH];
		sprintf(buffer, "%s", c);
		LOGI("file:%s", buffer);
		result.pushTail(buffer);
	}

	AAssetDir_close(assetDir);

	return result;
}

void Art::loadLevels(){
	List<char*> files = loadFilesList(LEVELS_PATH);
	levelsCount = files.getLength();
	char buffer[MAX_PATH];
	if(!files.isEmpty()){
		levels = new Texture*[levelsCount];
		char* file;
		bool exists = files.getHead(file);
		int i = 0;
		while(exists){
			sprintf(buffer, "%s/%s", LEVELS_PATH, file);
			levels[i] = loadPng(buffer);
			++i;
			delete[] file;
			exists = files.getNext(file);
		}
		files.clear();
	}
}

/*
 * 		Method return big image with all (<= MAX_LEVELS_COUNT) levels printed on it
 * and fill levelsTexCoords array with texture coords of levels.
 */
Texture* Art::makeTextureFromLevels(){
	LOGI("Art::makeTextureFromLevels");

	if(levelsCount <= 0){
		return NULL;
	}

	int w = LEVELS_ON_SIDE_COUNT*MAX_LEVEL_SIZE;
	int h = w;
	char* pixels = new char[w*h*4];
	memset(pixels, 0, w*h*4*sizeof(char));

	levelsTexCoords = new GLfloat*[levelsCount];
	float fWidth = (float) LEVELS_ON_SIDE_COUNT*MAX_LEVEL_SIZE;
	float fHeight = fWidth;

	for(int k = 0; k < levelsCount && k < MAX_LEVELS_COUNT; ++k){

		/*Compute position of this level*/
		int posX = k % LEVELS_ON_SIDE_COUNT;
		int posY = k / LEVELS_ON_SIDE_COUNT;

		int globalOffset =
				posX*MAX_LEVEL_SIZE*4 +
				posY*MAX_LEVEL_SIZE*MAX_LEVEL_SIZE*LEVELS_ON_SIDE_COUNT*4;

		Texture* currentLevel = levels[k];
		int iLevelWidth = currentLevel->width < MAX_LEVEL_SIZE ? currentLevel->width : MAX_LEVEL_SIZE;
		int iLevelHeight = currentLevel->height < MAX_LEVEL_SIZE ? currentLevel->height : MAX_LEVEL_SIZE;


		/*Print level image to big image*/
		for(int i = 0; i < iLevelHeight; ++i){
			memcpy(&(pixels[globalOffset + i*MAX_LEVEL_SIZE*LEVELS_ON_SIDE_COUNT*4]), &(currentLevel->pixels[i*currentLevel->width*4]), iLevelWidth*4);
		}

		/*Compute texture coords of this level in big texture*/
		float fLevelWidth = float(iLevelWidth) / (float)(MAX_LEVEL_SIZE*LEVELS_ON_SIDE_COUNT);
		float fLevelHeight = float(iLevelHeight) / (float)(MAX_LEVEL_SIZE*LEVELS_ON_SIDE_COUNT);
		float fX = (float)posX / float(LEVELS_ON_SIDE_COUNT);
		float fY = (float)posY / float(LEVELS_ON_SIDE_COUNT);

		GLfloat tempCoords[12] = {
			fX, fY, fX + fLevelWidth, fY, fX + fLevelWidth, fY + fLevelHeight,
			fX + fLevelWidth, fY + fLevelHeight, fX, fY + fLevelHeight, fX, fY
		};

		levelsTexCoords[k] = new GLfloat[12];

		for(int i = 0; i < 12; ++i){
			levelsTexCoords[k][i] = tempCoords[i];
		}

	}

	return new Texture(pixels, w, h);

}

#define btnTexSize 0.25

GLfloat Art::TEX_COORDS_BUTTON_LEFT[12] = {
	btnTexSize, btnTexSize, 0.0, btnTexSize, 0.0, 0.0,
	0.0, 0.0, btnTexSize, 0.0, btnTexSize, btnTexSize
};

GLfloat Art::TEX_COORDS_BUTTON_LEFT_PRESSED[12] = {
	2*btnTexSize, btnTexSize, btnTexSize, btnTexSize, btnTexSize, 0.0,
	btnTexSize, 0.0, 2*btnTexSize, 0.0, 2*btnTexSize, btnTexSize
};

GLfloat Art::TEX_COORDS_BUTTON_RIGHT[12] = {
	0.0, 0.0, btnTexSize, 0.0, btnTexSize, btnTexSize,
	btnTexSize, btnTexSize, 0.0, btnTexSize, 0.0, 0.0
};

GLfloat Art::TEX_COORDS_BUTTON_RIGHT_PRESSED[12] = {
	btnTexSize, 0.0, 2*btnTexSize, 0.0, 2*btnTexSize, btnTexSize,
	2*btnTexSize, btnTexSize, btnTexSize, btnTexSize, btnTexSize, 0.0
};

GLfloat Art::TEX_COORDS_BUTTON_UP[12] = {
	btnTexSize, 0.0, btnTexSize, btnTexSize, 0.0, btnTexSize,
	0.0, btnTexSize, 0.0, 0.0, btnTexSize, 0.0
};

GLfloat Art::TEX_COORDS_BUTTON_UP_PRESSED[12] = {
	2*btnTexSize, 0.0, 2*btnTexSize, btnTexSize, btnTexSize, btnTexSize,
	btnTexSize, btnTexSize, btnTexSize, 0.0, 2*btnTexSize, 0.0
};

GLfloat Art::TEX_COORDS_BUTTON_DOWN[12] = {
	0.0, btnTexSize, 0.0, 0.0, btnTexSize, 0.0,
	btnTexSize, 0.0, btnTexSize, btnTexSize, 0.0, btnTexSize
};

GLfloat Art::TEX_COORDS_BUTTON_DOWN_PRESSED[12] = {
	btnTexSize, btnTexSize, btnTexSize, 0.0, 2*btnTexSize, 0.0,
	2*btnTexSize, 0.0, 2*btnTexSize, btnTexSize, btnTexSize, btnTexSize
};

GLfloat Art::TEX_COORDS_BUTTON_RETRY[12] = {
	2*btnTexSize, 0.0, 3*btnTexSize, 0.0, 3*btnTexSize, btnTexSize,
	3*btnTexSize, btnTexSize, 2*btnTexSize, btnTexSize, 2*btnTexSize, 0.0
};

GLfloat Art::TEX_COORDS_BUTTON_LIST[12] = {
	3*btnTexSize, 0.0, 4*btnTexSize, 0.0, 4*btnTexSize, btnTexSize,
	4*btnTexSize, btnTexSize, 3*btnTexSize, btnTexSize, 3*btnTexSize, 0.0
};