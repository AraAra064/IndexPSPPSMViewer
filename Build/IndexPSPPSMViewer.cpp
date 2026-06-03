#include <iostream>
#include <fstream>
#include <deque>
#include <vector>
#include <string>
#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#include "IndexPSP/PSM.hpp"
#include "IndexPSP/CRAF.hpp"
#include "IndexPSP/SHTXPS.hpp"
#include "IndexPSP/ShadeDecoder.hpp"

#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"
#include "SFML/OpenGL.hpp"

struct UV
{
    float u;
    float v;
    
    UV()
    {
        u = 0.f;
        v = 0.f;
    }
    UV(float u, float v)
    {
        this->u = u;
        this->v = v;
    }
};

UV CalcUV(uint8_t u, uint8_t v)
{
    return UV((float)u / (float)256.f, (float)v / (float)256.f);
}

std::vector<uint8_t> LoadFile(std::string fileName)
{
    std::vector<uint8_t> data;
    std::ifstream rFile(fileName.c_str(), std::ios::binary);

    if (rFile.is_open())
    {
        uint32_t sz;
        rFile.seekg(0, std::ios::end);
        sz = (uint32_t)rFile.tellg();
        data.resize(sz);
        rFile.seekg(0, std::ios::beg);
        rFile.read((char*)&data[0], sz);
        rFile.close();
    }

    return data;
}

void ProcessInputs(sf::Vector3f& pos, float& rotation, float dT, bool& drawObjects, bool& fov)
{
    const float scaleR = 60.f, scaleS = 10.f;
    int rDir = 0, mDir = 0;
    auto mVec = sf::Vector3f(1.f, 0.5f, 1.f);

    using namespace sf::Keyboard;

    if (isKeyPressed(Key::Left))
    {
        rDir = 1;
    }
    else if (isKeyPressed(Key::Right))
    {
        rDir = -1;
    }

    if (isKeyPressed(Key::Up))
    {
        mDir = -1;
    }
    else if (isKeyPressed(Key::Down))
    {
        mDir = 1;
    }

    if (isKeyPressed(Key::O))
    {
        drawObjects = !drawObjects;
        //std::cout << "Object entry drawing has been toggled." << std::endl;
    }
    if (isKeyPressed(Key::P))
    {
        fov = !fov;
    }

    rotation += scaleR * rDir * dT;
    pos += (mDir * scaleS * mVec * dT);

    return;
}

void ConvertToOBJ(std::string fileName, std::vector<float>& vertices, std::vector<uint16_t>& indices)
{
    std::ofstream wFile(fileName.c_str());

    if (wFile.is_open())
    {
        wFile << "#" << fileName << '\n';

        for (int i = 0; i < vertices.size(); i += 3)
        {
            float v1 = vertices[i];
            float v2 = vertices[i + 1];
            float v3 = vertices[i + 2];
            wFile << "v " << v1 << ' ' << v2 << ' ' << v3 << '\n';
        }

        for (int i = 0; i < indices.size(); i += 4)
        {
            int i1 = indices[i] + 1;
            int i2 = indices[i + 1] + 1;
            int i3 = indices[i + 2] + 1;
            int i4 = indices[i + 3] + 1;
            wFile << "f " << i1 << ' ' << i2 << ' ' << i3 << ' ' << i4 << '\n';
        }

        wFile.close();
    }

    return;
}

std::vector<float> Section12(std::vector<uint8_t>& psmData)
{
    std::vector<float> v;

    auto off = IndexPSM::GetSectionOffset(psmData, 0x0C);
    auto count = *(uint32_t*)&psmData[off];
    off += 0x04;

    for (int i = 0; i < count; i++)
    {
        v.push_back(*(float*)&psmData[off + 4]);
        v.push_back(*(float*)&psmData[off + 8]);
        v.push_back(*(float*)&psmData[off]);
        off += sizeof(float) * 3;
    }

    return v;
}

//void DrawBoundary

void DrawWireObjectEX(float centreX, float y, float centreZ, float w, float h, uint32_t c)
{
    float szW = w * 0.5f, szH = h * 0.5f;
    sf::Vector3f verts[4];
    verts[0] = sf::Vector3f(centreX - szW, y, centreZ - szH);
    verts[1] = sf::Vector3f(centreX - szW, y, centreZ + szH);
    verts[2] = sf::Vector3f(centreX + szW, y, centreZ + szH);
    verts[3] = sf::Vector3f(centreX + szW, y, centreZ - szH);

    glLineWidth(3);

    glBegin(GL_LINE_LOOP);
    glColor3ub(GetRValue(c), GetGValue(c), GetBValue(c));
    glVertex3fv(&verts[0].x);
    glVertex3fv(&verts[1].x);
    glVertex3fv(&verts[2].x);
    glVertex3fv(&verts[3].x);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glColor3ub(GetRValue(c), GetGValue(c), GetBValue(c));
    glVertex3f(centreX, y, centreZ);
    glVertex3f(centreX, y + 10.f, centreZ);
    glEnd();
}

void DrawWireObject(float centreX, float y, float centreZ, float size, uint32_t c)
{
    float sz = size * 0.5f;
    sf::Vector3f verts[4];
    verts[0] = sf::Vector3f(centreX - sz, y, centreZ - sz);
    verts[1] = sf::Vector3f(centreX - sz, y, centreZ + sz);
    verts[2] = sf::Vector3f(centreX + sz, y, centreZ + sz);
    verts[3] = sf::Vector3f(centreX + sz, y, centreZ - sz);

    glLineWidth(3);

    glBegin(GL_LINE_LOOP);
    glColor3ub(GetRValue(c), GetGValue(c), GetBValue(c));
    glVertex3fv(&verts[0].x);
    glVertex3fv(&verts[1].x);
    glVertex3fv(&verts[2].x);
    glVertex3fv(&verts[3].x);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glColor3ub(GetRValue(c), GetGValue(c), GetBValue(c));
    glVertex3f(centreX, y, centreZ);
    glVertex3f(centreX, y + 10.f, centreZ);
    glEnd();
}

//Testing
std::vector<uint8_t> NewFile(std::vector<uint8_t>& craf)
{
    std::vector<uint8_t> psm = IndexCRAF::GetFileData(craf, 0);
    auto draw = IndexPSM::GetDrawOrder(psm);
    auto verts = IndexPSM::GetVertexEntries(psm);
    auto faces = IndexPSM::GetFaceEntries(psm);
    auto colours = IndexPSM::GetColourEntries(psm);
    auto uvs = IndexPSM::GetUVEntries(psm);
    auto hd = IndexPSM::GetHitData2(psm);
    auto objects = IndexPSM::GetObjectEntries(psm);
    std::vector<IndexPSM::ObjectEntry> obj2;

    for (auto o : objects)
    {
        int s = 1;
        if ((s = o.getScale()) != 0 && o.type == IndexPSM::ModelID::WindmillDay)
        {
            o.type = IndexPSM::ModelID::WindmillNight;
            o.setScale(s / 2);
        }
        obj2.push_back(o);
    }

    psm = IndexPSM::CreatePSM(draw, verts, faces, colours, uvs, hd, obj2);

    return psm;
}

int main()
{
    std::string fn;
    #ifndef _DEBUG
    std::cout << "Enter name of CRAF file to load:" << std::flush;
    std::getline(std::cin, fn);
    #else 
    fn = "File3.CRAF";
    #endif
    auto crafData = LoadFile(fn);

    if (crafData.empty())
    {
        return 1;
    }

    std::deque<std::vector<uint8_t>> files = IndexCRAF::GetFiles(crafData);
    auto psmData = files[0]; //IndexCRAF::GetFileData(crafData, 0);
    std::deque<sf::Texture> textures;
    uint32_t nTex = files.size(); //IndexCRAF::GetNumberOfFiles(crafData);
    for (uint32_t i = 1; i < nTex; i++)
    {
        auto shtxpsFile = files[i];//IndexCRAF::GetFileData(crafData, i);
        uint32_t width, height;
        auto data = IndexSHTXPS::GetPixelData(shtxpsFile, width, height, true);
        bool loaded = (!data.empty() && width != 0 && height != 0);
        
        if (loaded)
        {
            sf::Texture tex(sf::Vector2u(width, height));
            tex.update((uint8_t*)data.data());
            textures.push_back(tex);
        }
    }

    auto drawOrder = IndexPSM::GetDrawOrder(psmData);
    auto verts = IndexPSM::GetVertices(psmData);
    auto indic = IndexPSM::GetIndices(psmData);
    auto uvTable = IndexPSM::GetUVTable(psmData);
    auto uvIDs = IndexPSM::GetUVIDList(psmData);
    auto texIDs = IndexPSM::GetTexIDList(psmData);
    auto colourTable = IndexPSM::GetColourTable(psmData);
    colourTable.push_back(colourTable.back()); //temp fix2 - some maps go out of bounds on the ct on the +3 part
    auto colourIDs = IndexPSM::GetColourIDList(psmData);
    auto objects = IndexPSM::GetObjectEntries(psmData);
    auto origin = IndexPSM::GetHitDataOrigin(psmData);

    #ifndef _DEBUG
    ConvertToOBJ("Model.obj", verts, indic);
    #endif

    std::cout << "DrawCount=" << drawOrder.size() << std::endl;
    std::cout << "VertCount=" << IndexPSM::GetNumVertices(psmData) << std::endl;
    std::cout << "FaceCount=" << IndexPSM::GetNumFaces(psmData) << std::endl;
    std::cout << "UVTableCount=" << IndexPSM::GetNumUVs(psmData) << std::endl;
    std::cout << "ColourTableCount=" << IndexPSM::GetNumColours(psmData) << std::endl;
    std::cout << "TexCount=" << textures.size() << std::endl;
    std::cout << "HitDataOrigin=(" << origin.first << ", " << origin.second << ")" << std::endl;
    std::cout << "HitDataSZ=" << IndexPSM::GetHitDataSZ(psmData) << std::endl;
    std::cout << "ObjectEntryCount=" << objects.size() << std::endl;
    //std::cout << objects[0x24].isNull() << std::endl;

    bool drawObjects = false, highFOV = false;
    float rotation = 0.f;
    sf::Vector3f pos = {2.f, 0.5f, 2.f};
    sf::Clock c;
    float dT = 0.f;

    float scale = 3.f;
    sf::Vector2u resolution(420 * scale, 272 * scale);
    sf::ContextSettings ct;
    ct.depthBits = 32;
    ct.antiAliasingLevel = 4;
    sf::RenderWindow window(sf::VideoMode(resolution), "PSM Model Viewer", sf::State::Windowed, ct);
    window.setFramerateLimit(60);

    sf::Font font("monofonto rg.otf");
    sf::Text text(font);

    window.setActive();
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_AUTO_NORMAL);
    window.setActive(false);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            using namespace sf::Keyboard;

            if (event->is<sf::Event::Closed>() || isKeyPressed(Key::Escape))
                window.close();
        }

        dT = c.restart().asSeconds();
        window.clear();
        window.setActive();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(highFOV ? 90.f : 60.f, resolution.x / resolution.y, 0.01f, 500.f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(pos.x, pos.y, pos.z, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);
        glRotatef(rotation, 0.f, 1.f, 0.f);
        
        ProcessInputs(pos, rotation, dT, drawObjects, highFOV);

        if (rotation < 0.f)
        {
            rotation = 360.f - rotation;
        }
        else if (rotation > 360.f)
        {
            rotation -= 360.f;
        }

        for (IndexPSM::DrawData& dd : drawOrder)
        {
            uint16_t nFaces = dd.startFace + dd.numFaces;
            for (int i = dd.startFace; i < nFaces; i++)
            {
                auto cID = colourIDs[i];
                auto texID = texIDs[i];
                auto uvID = uvIDs[i];
                auto uvIndex = (texID != 0xFF ? uvID : uvIDs.back()) * 8; //temp fix1 - why 0xFF tex ID?????
                sf::Texture& tex = textures[texID];
                sf::Texture::bind(&tex);
                glBegin(GL_QUADS); //sorry no buffers
                //v0
                int j = indic[i * 4] * 3;
                UV uv = CalcUV(uvTable[uvIndex], uvTable[uvIndex + 1]);
                glTexCoord2f(uv.u, uv.v);
                uint32_t c = colourTable[cID];
                glColor3ub(GetRValue(c), GetGValue(c), GetBValue(c));
                glVertex3f(verts[j], verts[j + 1], verts[j + 2]);
                //v1
                j = indic[(i * 4) + 1] * 3;
                uv = CalcUV(uvTable[uvIndex + 2], uvTable[uvIndex + 3]);
                glTexCoord2f(uv.u, uv.v);
                c = colourTable[cID + 1];
                glColor3ub(GetRValue(c), GetGValue(c), GetBValue(c));
                glVertex3f(verts[j], verts[j + 1], verts[j + 2]);
                //v2
                j = indic[(i * 4) + 2] * 3;
                uv = CalcUV(uvTable[uvIndex + 4], uvTable[uvIndex + 5]);
                glTexCoord2f(uv.u, uv.v);
                c = colourTable[cID + 2];
                glColor3ub(GetRValue(c), GetGValue(c), GetBValue(c));
                glVertex3f(verts[j], verts[j + 1], verts[j + 2]);
                //v3
                j = indic[(i * 4) + 3] * 3;
                uv = CalcUV(uvTable[uvIndex + 6], uvTable[uvIndex + 7]);
                glTexCoord2f(uv.u, uv.v);
                c = colourTable[cID + 3];
                glColor3ub(GetRValue(c), GetGValue(c), GetBValue(c));
                glVertex3f(verts[j], verts[j + 1], verts[j + 2]);
                glEnd();
            }
        }

        sf::Texture::bind(nullptr);

        if (drawObjects)
        {
            //glDisable(GL_DEPTH_TEST);
            uint32_t col[] = {0x00FF0000, 0x0000FF00, 0x000000FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF };
            for (auto& entry : objects)
            {
                uint16 id = entry.prop1;
                float p = entry.x + entry.y + entry.z;
                if (id != 0x00 || p != 0.f)
                {
                    DrawWireObject(entry.x, entry.y + 0.1f, entry.z, 1.f, id < 7 ? col[id] : 0x00FFFFFF);
                }
            }
            //glEnable(GL_DEPTH_TEST);
        }

        //float y = *(float*)&psmData[IndexPSM::GetHitDataOffset(psmData) + 0x00];
        //float z = *(float*)&psmData[IndexPSM::GetHitDataOffset(psmData) + 0x04];
        //float x = *(float*)&psmData[IndexPSM::GetHitDataOffset(psmData) + 0x08];

        //x, z
        DrawWireObjectEX(0, 0, 0, -origin.first, -origin.second, 0x0000FFFF);

        window.setActive(false);
        window.pushGLStates();

        std::string s = "(" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z) + "), ";
        s += std::to_string(rotation) + " deg\n";
        s += "LEFT/RIGHT=Rotate\n";
        s += "UP/DOWN=Move forward/back\n";
        s += "O=Toggle object view\n";
        s += "ESC=Exit\n";
        text.setString(s);
        window.draw(text);

        window.popGLStates();
        window.display();
    }

    window.close();
}