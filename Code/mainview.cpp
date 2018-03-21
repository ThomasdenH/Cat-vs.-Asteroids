#include "mainview.h"
#include "model.h"
#include "vertex.h"

#include <math.h>
#include <QDateTime>

/**
 * @brief MainView::MainView
 *
 * Constructor of MainView
 *
 * @param parent
 */
MainView::MainView(QWidget *parent) : QOpenGLWidget(parent) {
    qDebug() << "MainView constructor";

    game = Game();
    time = 0.0;
    scoreDisplay = parent->findChild<QLCDNumber*>("scoreDisplay");

    connect(&timer, SIGNAL(timeout()), this, SLOT(update()));
}

/**
 * @brief MainView::~MainView
 *
 * Destructor of MainView
 * This is the last function called, before exit of the program
 * Use this to clean up your variables, buffers etc.
 *
 */
MainView::~MainView() {
    debugLogger->stopLogging();

    qDebug() << "MainView destructor";

    glDeleteTextures(1, &cat.texturePtr);
    glDeleteTextures(1, &sphere.texturePtr);
    destroyModelBuffers();
}

// --- OpenGL initialization

/**
 * @brief MainView::initializeGL
 *
 * Called upon OpenGL initialization
 * Attaches a debugger and calls other init functions
 */
void MainView::initializeGL() {
    qDebug() << ":: Initializing OpenGL";
    initializeOpenGLFunctions();

    debugLogger = new QOpenGLDebugLogger();
    connect( debugLogger, SIGNAL( messageLogged( QOpenGLDebugMessage ) ),
             this, SLOT( onMessageLogged( QOpenGLDebugMessage ) ), Qt::DirectConnection );

    if ( debugLogger->initialize() ) {
        qDebug() << ":: Logging initialized";
        debugLogger->startLogging( QOpenGLDebugLogger::SynchronousLogging );
        debugLogger->enableMessages();
    }

    QString glVersion;
    glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    qDebug() << ":: Using OpenGL" << qPrintable(glVersion);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.0, 1.0, 0.0, 1.0);

    createShaderProgram();
    loadObjects();
    loadTextures();

    // Initialize transformations
    updateProjectionTransform();
}

void MainView::createShaderProgram()
{
    // Create Phong Shader program
    phongShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                           ":/shaders/vertshader_phong.glsl");
    phongShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                           ":/shaders/fragshader_phong.glsl");
    phongShaderProgram.link();

    // Get the uniforms for the phong shader.
    uniformModelViewTransformPhong  = phongShaderProgram.uniformLocation("modelViewTransform");
    uniformProjectionTransformPhong = phongShaderProgram.uniformLocation("projectionTransform");
    uniformNormalTransformPhong     = phongShaderProgram.uniformLocation("normalTransform");
    uniformMaterialPhong            = phongShaderProgram.uniformLocation("material");
    uniformLightPositionPhong       = phongShaderProgram.uniformLocation("lightPosition");
    uniformLightColourPhong         = phongShaderProgram.uniformLocation("lightColour");
    uniformTextureSamplerPhong      = phongShaderProgram.uniformLocation("textureSampler");
}

void MainView::loadObjects() {
    // Load the cat
    Model asteroidModel(":/models/cat.obj");
    asteroidModel.unitize();
    QVector<float> meshData = asteroidModel.getVNTInterleaved();

    cat.meshSize = asteroidModel.getVertices().size();

    // Generate VAO
    glGenVertexArrays(1, &cat.meshVAO);
    glBindVertexArray(cat.meshVAO);

    // Generate VBO
    glGenBuffers(1, &cat.meshVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cat.meshVBO);

    // Write the data to the buffer
    glBufferData(GL_ARRAY_BUFFER, meshData.size() * sizeof(float), meshData.data(), GL_STATIC_DRAW);

    // Set vertex coordinates to location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    // Set vertex normals to location 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Set vertex texture coordinates to location 2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Load the sphere
    Model sphereModel(":/models/asteroid.obj");
    sphereModel.unitize();
    QVector<float> sphereMeshData = sphereModel.getVNTInterleaved();
    sphere.meshSize = sphereModel.getVertices().size();

    // Generate VAO
    glGenVertexArrays(1, &sphere.meshVAO);
    glBindVertexArray(sphere.meshVAO);

    // Generate VBO
    glGenBuffers(1, &sphere.meshVBO);
    glBindBuffer(GL_ARRAY_BUFFER,sphere.meshVBO);

    // Write the data to the buffer
    glBufferData(GL_ARRAY_BUFFER, sphereMeshData.size() * sizeof(float), sphereMeshData.data(), GL_STATIC_DRAW);

    // Set vertex coordinates to location 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
    glEnableVertexAttribArray(0);

    // Set vertex normals to location 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Set vertex texture coordinates to location 2
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void MainView::loadTextures()
{
    glGenTextures(1, &cat.texturePtr);
    glGenTextures(1, &sphere.texturePtr);
    loadTexture(":/textures/cat_diff.png", cat.texturePtr);
    loadTexture(":/textures/asteroid.png", sphere.texturePtr);
}

void MainView::loadTexture(QString file, GLuint texturePtr)
{
    // Set texture parameters.
    glBindTexture(GL_TEXTURE_2D, texturePtr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Push image data to texture.
    QImage image(file);
    QVector<quint8> imageData = imageToBytes(image);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width(), image.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
}

// --- OpenGL drawing

/**
 * @brief MainView::paintGL
 *
 * Actual function used for drawing to the screen
 *
 */
void MainView::paintGL() {
    // Update the game
    game.update(1.0 / 60.0);
    scoreDisplay->display((int) game.getScore());

    // Clear the screen before rendering
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    phongShaderProgram.bind();
    updatePhongUniforms();

    // Draw the cat
    drawCat();

    // Draw the boxes
    for (unsigned i = 0; i < game.getBoxCount(); i++) {
        drawBox(i);
    }

    phongShaderProgram.release();
    timer.start(1000.0 / 60.0);
}

void MainView::drawCat() {
    QMatrix4x4 catTransform = QMatrix();
    QVector3D catLocation = game.getCatLocation();
    catTransform.setToIdentity();
    catTransform.translate(catLocation);
    catTransform.rotate(90, 0, 1, 0);
    catTransform.rotate(game.getCatRotation(), 1, 0, 0);

    glUniformMatrix4fv(uniformModelViewTransformPhong, 1, GL_FALSE, catTransform.data());
    glUniformMatrix3fv(uniformNormalTransformPhong, 1, GL_FALSE, catTransform.normalMatrix().data());

    // Set the texture and draw the mesh.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cat.texturePtr);

    glBindVertexArray(cat.meshVAO);
    glDrawArrays(GL_TRIANGLES, 0, cat.meshSize);
}

void MainView::drawBox(unsigned i) {
    QMatrix4x4 boxTransform = QMatrix();
    QVector3D boxLocation = game.getBoxLocation(i);
    boxTransform.translate(boxLocation);
    double time = (boxLocation.z() + boxLocation.x() * 10.0) / 30.0;
    boxTransform.rotate(90, cos(time), cos(time + 10.0), cos(time + 20.0));

    glUniformMatrix4fv(uniformModelViewTransformPhong, 1, GL_FALSE, boxTransform.data());
    glUniformMatrix3fv(uniformNormalTransformPhong, 1, GL_FALSE, boxTransform.normalMatrix().data());

    // Set the texture and draw the mesh.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sphere.texturePtr);

    glBindVertexArray(sphere.meshVAO);
    glDrawArrays(GL_TRIANGLES, 0, sphere.meshSize);
}

/**
 * @brief MainView::resizeGL
 *
 * Called upon resizing of the screen
 *
 * @param newWidth
 * @param newHeight
 */
void MainView::resizeGL(int newWidth, int newHeight)
{
    Q_UNUSED(newWidth)
    Q_UNUSED(newHeight)
    updateProjectionTransform();
}

void MainView::updatePhongUniforms()
{
    glUniformMatrix4fv(uniformProjectionTransformPhong, 1, GL_FALSE, projectionTransform.data());

    glUniform4fv(uniformMaterialPhong, 1, &material[0]);
    glUniform3fv(uniformLightPositionPhong, 1, &lightPosition[0]);
    glUniform3fv(uniformLightColourPhong, 1, &lightColour[0]);

    glUniform1i(uniformTextureSamplerPhong, 0);
}

void MainView::updateProjectionTransform()
{
    float aspect_ratio = static_cast<float>(width()) / static_cast<float>(height());
    projectionTransform.setToIdentity();
    projectionTransform.perspective(60, aspect_ratio, 0.2, 100);
}

// --- OpenGL cleanup helpers

void MainView::destroyModelBuffers()
{
    glDeleteBuffers(1, &cat.meshVBO);
    glDeleteVertexArrays(1, &cat.meshVAO);
    glDeleteBuffers(1, &sphere.meshVBO);
    glDeleteVertexArrays(1, &sphere.meshVAO);
}

// --- Private helpers

/**
 * @brief MainView::onMessageLogged
 *
 * OpenGL logging function, do not change
 *
 * @param Message
 */
void MainView::onMessageLogged( QOpenGLDebugMessage Message ) {
    qDebug() << " → Log:" << Message;
}

