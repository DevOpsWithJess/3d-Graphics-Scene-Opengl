///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// SNHU CS330 Comp Graphic and Visualization
// Jessica Johnson
// 07/20/25
// 
// 3-3 Milestone Two
// Created the lipgloss from 4 different shapes. Two cylinders, one sphere, 
// and, one cone.
// 
// 4-2 Milestone Three
// Added camera interaction using keyboard and mouse
// Implemented movement, ,mouse orbitting, and scroll zoom
// 
// 5-3 Milestone Four
// Applied Textures to lipgloss parts using image files. 
// Implemented complex texturing techniques like UV scaling. 
//
// 6-1 Milestone Five
// Added Phong lighting to the scene with multiple light sources and materials. 
// Materials control how each object looks with ambient, diffuse, and specular 
// settings, and lights add direction plus their own ambient, diffuse, and 
// specular values. The scene now sets up these lights and materials in the 
// shader so the lip gloss model has more realistic shading. This meets the 
// Module Six lighting requirements.
//
// 7-1 Final Project
// Added the remaing 4 objects into the scene. Compact mirror, nail polish,
// and two brushes.
// Finalized texturing and colors to better match original image.
// 
// Overall, CS330 has taught me how 3D graphics are built from basic
// geometric shapes, transformations, and math like concepts. I learned how to
// apply scale, rotation, translation, textures, UV mapping, shaders, lighting
// models, camera controls, and general scence composition. -Jessica Johnson
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  LoadSceneTextures()
 * Load image files
 * Convert the image files to OpenGl textures and bind
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool ok = false;

	// Tube and base piece: glossy plastic(tiled)
	ok = CreateGLTexture("../../Utilities/textures/glossy_plastic.jpg", "gloss_body");
	// Cap: Brushed gold
	ok = CreateGLTexture("../../Utilities/textures/gold-seamless-texture.jpg", "gloss_cap");
	// Brush bristles texture
	ok = CreateGLTexture("../../Utilities/textures/brush_bristles.jpg", "brush_bristles");
	// Floor texture
	ok = CreateGLTexture("../../Utilities/textures/knife_handle.jpg", "floor_tex");
	// Mirror texture
	ok = CreateGLTexture("../../Utilities/textures/stainless_end.jpg", "mirror");
	// Powder texture
	ok = CreateGLTexture("../../Utilities/textures/breadcrust.jpg", "powder");

	// Bind the loaded textures active on texture slots
	BindGLTextures();
}
/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method defines material properties for the objects
 *  in the scene (ambient, diffuse, specular, shinyness).
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL glossMaterial;
	glossMaterial.ambientStrength = 0.2f;
	glossMaterial.ambientColor = glm::vec3(1.0f, 0.7f, 0.8f);
	glossMaterial.diffuseColor = glm::vec3(1.0f, 0.7f, 0.8f);
	glossMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glossMaterial.shininess = 10.0f;
	glossMaterial.tag = "gloss_body";

	m_objectMaterials.push_back(glossMaterial);

	OBJECT_MATERIAL capMaterial;
	capMaterial.ambientStrength = 0.3f;
	capMaterial.ambientColor = glm::vec3(1.0f, 0.84f, 0.0f);
	capMaterial.diffuseColor = glm::vec3(1.0f, 0.84f, 0.0f);
	capMaterial.specularColor = glm::vec3(1.0f, 1.0f, 0.8f);
	capMaterial.shininess = 128.0f;
	capMaterial.tag = "gloss_cap";

	m_objectMaterials.push_back(capMaterial);

	OBJECT_MATERIAL floorMat;
	floorMat.ambientStrength = 0.2f;
	floorMat.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	floorMat.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	floorMat.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	floorMat.shininess = 8.0f;
	floorMat.tag = "floor";
	m_objectMaterials.push_back(floorMat);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method sets up lighting parameters in the shader.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	if (!m_pShaderManager) return;

	// Turn lighting on
	m_pShaderManager->setBoolValue("bUseLighting", true);

	// Light 0
	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Light 1
	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	// Light 2
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.2f);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load textures
	LoadSceneTextures();

	// Materials
	DefineObjectMaterials();

	// Lighting
	SetupSceneLights();

	// Load each Mesh
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/


	/****************************************************************/
	/****************************************************************/

	//////////////// Plane ////////////////
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("floor");
	SetTextureUVScale(4.0f, 4.0f);
	SetShaderTexture("floor_tex");

	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/
	/****************************************************************/

	//////////////// Lip Gloss ////////////////

	// Lip Gloss: Main Tube (1 of 4 Pieces)
	
	scaleXYZ = glm::vec3(0.7f, 3.8f, 0.7f);
	XrotationDegrees = 100.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -20.0f;
	positionXYZ = glm::vec3(-6.5f, 1.5f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(2.0f, 1.0f);
	SetShaderTexture("gloss_body");
	SetShaderMaterial("gloss_body");
	m_basicMeshes->DrawCylinderMesh();
	SetTextureUVScale(1.0f, 1.0f);

	/****************************************************************/


	// Lip Gloss: Lid (2 of 4 Pieces)
	scaleXYZ = glm::vec3(0.71f, 0.9f, 0.71f);
	XrotationDegrees = 100.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = -18.5f;
	positionXYZ = glm::vec3(-6.12f, 2.2f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("gloss_cap");
	SetShaderMaterial("gloss_cap");
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/


	// Lip Gloss: Base Sphere (3 of 4 Pieces)
	scaleXYZ = glm::vec3(0.4f, 0.6f, 0.4f);
	XrotationDegrees = 100.0f;
	YrotationDegrees = -15.0f;
	ZrotationDegrees = -18.5f;
	positionXYZ = glm::vec3(-6.5f, -1.2f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("gloss_body");
	SetShaderMaterial("gloss_body");
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/


	// Lip Gloss: Cone Base (4 of 4 Pieces)
	scaleXYZ = glm::vec3(0.5f, 0.3f, 0.5f);
	XrotationDegrees = 120.0f;
	YrotationDegrees = 40.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-4.05f, 1.8f, 7.8f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(1.8f, 1.0f);
	SetShaderTexture("gloss_body");
	SetShaderMaterial("gloss_body");
	m_basicMeshes->DrawConeMesh();
	SetTextureUVScale(1.0f, 1.0f);

	/****************************************************************/
	/****************************************************************/

	/****************************************************************/
	/****************************************************************/

	//////////////// Compact ////////////////

	// Compact: Bottom Base (1 of 2 pieces)
	scaleXYZ = glm::vec3(1.8f, 0.15f, 1.8f);
	positionXYZ = glm::vec3(-3.0f, 0.0f, 5.8f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("powder");
	m_basicMeshes->DrawCylinderMesh();

	// Compact: Top Lid (2 of 2 pieces)
	scaleXYZ = glm::vec3(1.8f, 0.10f, 1.8f);
	XrotationDegrees = -70.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-3.0f, 1.4f, 5.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("mirror");
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/****************************************************************/

	/****************************************************************/
	/****************************************************************/
	
	//////////////// Nail Polish ////////////////

	// Nail Polish: Bottle (1 of 2 pieces)
	scaleXYZ = glm::vec3(0.6f, 1.2f, 0.6f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 0.0f, 6.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.6f, 0.2f, 0.8f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Nail Polish: Cap (2 of 2 pieces)
	scaleXYZ = glm::vec3(0.3f, 0.9f, 0.45f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 1.0f, 6.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("gloss_cap");
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/****************************************************************/

	/****************************************************************/
	/****************************************************************/

	//////////////// Large Brush //////////////////

	// Large Brush: Handle (1 of 2 pieces)
	scaleXYZ = glm::vec3(0.45f, 3.8f, 0.35f);
	XrotationDegrees = -7.0f;
	YrotationDegrees = -35.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(2.0f, 0.55f, 3.7f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Large Brush: Bristles (2 of 2 pieces)
	scaleXYZ = glm::vec3(0.8f, 0.18f, 0.5f);
	XrotationDegrees = 45.0f;
	YrotationDegrees = -35.0f;
	ZrotationDegrees = -15.0f;
	positionXYZ = glm::vec3(2.6f, 0.40f, 4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("brush_bristles");

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/****************************************************************/

	/****************************************************************/
	/****************************************************************/

	//////////////// Small Brush //////////////////

	// Small Brush: Handle (1 of 2 pieces)
	scaleXYZ = glm::vec3(0.2f, 3.6f, 0.2f);
	XrotationDegrees = -7.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(3.9f, 0.4f, 2.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Small Brush: Bristles (2 of 2 pieces)
	scaleXYZ = glm::vec3(0.35f, 0.1f, 0.25f);
	XrotationDegrees = -90.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = -35.0f;
	positionXYZ = glm::vec3(4.25f, 0.38f, 2.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderTexture("brush_bristles");

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/****************************************************************/
	/****************************************************************/
	/****************************************************************/

}
