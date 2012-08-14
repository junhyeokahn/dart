#include "MyWindow.h"
#include "dynamics/BodyNodeDynamics.h"
#include "dynamics/ContactDynamics.h"
#include "kinematics/Dof.h"
#include "kinematics/Shape.h"
#include "utils/UtilsMath.h"
#include "utils/Timer.h"
#include "yui/GLFuncs.h"
#include <cstdio>
#include "kinematics/BodyNode.h"
#include <fstream>

using namespace Eigen;
using namespace kinematics;
using namespace utils;
using namespace integration;
using namespace dynamics;

void MyWindow::initDyn()
{
    mDofs.resize(mSkels.size());
    mDofVels.resize(mSkels.size());
    mStoredDofs.resize(mSkels.size());
    mStoredDofVels.resize(mSkels.size());

    for (unsigned int i = 0; i < mSkels.size(); i++) {
        mDofs[i].resize(mSkels[i]->getNumDofs());
        mDofVels[i].resize(mSkels[i]->getNumDofs());
        mDofs[i].setZero();
        mDofVels[i].setZero();
        mStoredDofs[i].resize(mSkels[i]->getNumDofs());
        mStoredDofVels[i].resize(mSkels[i]->getNumDofs());
    }
    
    mDofs[0][1] = -0.9; // ground level
    // squat pose
    mDofs[1][1] = -0.48;
    mDofs[1][6] = 1.3;
    mDofs[1][9] = -2.3;
    mDofs[1][10] = 1;
    mDofs[1][13] = 1.3;
    mDofs[1][16] = -2.3;
    mDofs[1][17] = 1;
    mDofs[1][21] = -1;
    mDofs[1][26] = 1.0;
    mDofs[1][28] = -0.4;
    mDofs[1][32] = 1.0;
    mDofs[1][34] = 0.4;
    
    for (unsigned int i = 0; i < mSkels.size(); i++) {
        mSkels[i]->initDynamics();
        mSkels[i]->setPose(mDofs[i], false, false);
        mSkels[i]->computeDynamics(mGravity, mDofVels[i], false);
    }
    mSkels[0]->setImmobileState(true);

    int nDof = mSkels[1]->getNumDofs();
    mCollisionHandle = new dynamics::ContactDynamics(mSkels, mTimeStep);
    mController = new Controller(mSkels[1], mCollisionHandle, mTimeStep);
     
    for (int i = 0; i < nDof; i++)
        mController->setDesiredDof(i, mController->getSkel()->getDof(i)->getValue());
    
    // modfify desired pose to a launch pose
    mController->setDesiredDof(6, -0.1);
    mController->setDesiredDof(9, 0.2);
    mController->setDesiredDof(10, 0.1);
    mController->setDesiredDof(12, 0.8);
    mController->setDesiredDof(13, -0.1);
    mController->setDesiredDof(16, 0.2);
    mController->setDesiredDof(17, 0.1);
    mController->setDesiredDof(19, 0.8);
    mController->setDesiredDof(21, 0.1);
    mController->setDesiredDof(26, 2.2);
    mController->setDesiredDof(28, -1.8);
    mController->setDesiredDof(32, 2.2);
    mController->setDesiredDof(34, 1.8);
    
    // record history of momentum for plotting in Octave
    mController->mLinHist.push_back(Vector3d::Zero());
    mController->mAngHist.push_back(Vector3d::Zero());
}

VectorXd MyWindow::getState() {
    VectorXd state(mIndices.back() * 2);    
    for (unsigned int i = 0; i < mSkels.size(); i++) {
        int start = mIndices[i] * 2;
        int size = mDofs[i].size();
        state.segment(start, size) = mDofs[i];
        state.segment(start + size, size) = mDofVels[i];
    }
    return state;
}

VectorXd MyWindow::evalDeriv() {
    setPose();
    VectorXd deriv = VectorXd::Zero(mIndices.back() * 2);    
    for (unsigned int i = 0; i < mSkels.size(); i++) {
        if (mSkels[i]->getImmobileState())
            continue;
        int start = mIndices[i] * 2;
        int size = mDofs[i].size();
        VectorXd qddot = mSkels[i]->getInvMassMatrix() * (-mSkels[i]->getCombinedVector() + mSkels[i]->getExternalForces() + mCollisionHandle->getConstraintForce(i) + mSkels[i]->getInternalForces());
        mSkels[i]->clampRotation(mDofs[i], mDofVels[i]);
        deriv.segment(start, size) = mDofVels[i]; // set velocities
        deriv.segment(start + size, size) = qddot; // set qddot (accelerations)
    }
    return deriv;
}


void MyWindow::setState(VectorXd newState) {
    for (unsigned int i = 0; i < mSkels.size(); i++) {
        int start = mIndices[i] * 2;
        int size = mDofs[i].size();
        mDofs[i] = newState.segment(start, size);
        mDofVels[i] = newState.segment(start + size, size);
    }
}

void MyWindow::setPose() {
    for (unsigned int i = 0; i < mSkels.size(); i++) {
        if (mSkels[i]->getImmobileState()) {
            mSkels[i]->setPose(mDofs[i], true, false);
        } else {
            mSkels[i]->setPose(mDofs[i], false, true);
            mSkels[i]->computeDynamics(mGravity, mDofVels[i], true);    
        }
    }
    mCollisionHandle->applyContactForces();
    mController->setConstrForces(mCollisionHandle->getConstraintForce(1));
}

void MyWindow::displayTimer(int _val)
{
    int numIter = mDisplayTimeout / (mTimeStep * 1000);
    if (mPlay) {
        mPlayFrame += 30;
        if (mPlayFrame >= mBakedStates.size())
            mPlayFrame = 0;
        glutPostRedisplay();
        glutTimerFunc(mDisplayTimeout, refreshTimer, _val);        
    }else if (mSim) {
        //  static Timer tSim("Simulation");
        for (int i = 0; i < numIter; i++) {
            //            tSim.startTimer();
            // push on spine, for perturbation test
            static_cast<BodyNodeDynamics*>(mSkels[1]->getNode(8))->addExtForce(Vector3d(0.0, 0.0, 0.0), mForce);

            mControlBias = mController->computeTorques(mDofs[1], mDofVels[1]);
            sampleControl();
            mSkels[1]->setInternalForces(mBestTorques);
            mIntegrator.integrate(this, mTimeStep);

            Vector3d lin = evalLinMomentum(mSkels[1], mDofVels[1]);
            mController->mLinHist.push_back(lin);
            Vector3d ang = evalAngMomentum(mSkels[1], mDofVels[1]);
            mController->mAngHist.push_back(ang);

            //            tSim.stopTimer();
            //            tSim.printScreen();
            bake();
            cout << "iter " << i + mSimFrame << endl;
        }
        // for perturbation test
        mImpulseDuration--;
        if (mImpulseDuration <= 0) {
            mImpulseDuration = 0;
            mForce.setZero();
        }

        mSimFrame += numIter;

        glutPostRedisplay();
        glutTimerFunc(mDisplayTimeout, refreshTimer, _val);
    }
}

void MyWindow::draw()
{
    glDisable(GL_LIGHTING);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    if (!mSim) {
        if (mPlayFrame < mBakedStates.size()) {
            for (unsigned int i = 0; i < mSkels.size(); i++) {
                int start = mIndices[i];
                int size = mDofs[i].size();
                mSkels[i]->setPose(mBakedStates[mPlayFrame].segment(start, size), true, false);
            }
            Vector3d com = mSkels[1]->getWorldCOM();
            mRI->setPenColor(Vector3d(0.8, 0.8, 0.2));
            mRI->pushMatrix();
            glTranslated(com[0], com[1], com[2]);
            mRI->drawEllipsoid(Vector3d(0.05, 0.05, 0.05));
            mRI->popMatrix();

            if (mShowMarkers) {
                int sumDofs = mIndices[mSkels.size()]; 
                int nContact = (mBakedStates[mPlayFrame].size() - sumDofs) / 6;
                for (int i = 0; i < nContact; i++) {
                    Vector3d v = mBakedStates[mPlayFrame].segment(sumDofs + i * 6, 3);
                    Vector3d f = mBakedStates[mPlayFrame].segment(sumDofs + i * 6 + 3, 3) / 100.0;
                    mRI->setPenColor(Vector3d(0.2, 0.2, 0.8));
                    glBegin(GL_LINES);
                    glVertex3f(v[0], v[1], v[2]);
                    glVertex3f(v[0] + f[0], v[1] + f[1], v[2] + f[2]);
                    glEnd();
                    mRI->pushMatrix();
                    glTranslated(v[0], v[1], v[2]);
                    mRI->drawEllipsoid(Vector3d(0.02, 0.02, 0.02));
                    mRI->popMatrix();
                }
            }
        }
    }else{
        if (mShowMarkers) {
            for (int k = 0; k < mCollisionHandle->getCollisionChecker()->getNumContact(); k++) {
                Vector3d  v = mCollisionHandle->getCollisionChecker()->getContact(k).point;
                Vector3d n = mCollisionHandle->getCollisionChecker()->getContact(k).normal / 10.0;
                Vector3d f = mCollisionHandle->getCollisionChecker()->getContact(k).force / 100.0;

                mRI->setPenColor(Vector3d(0.2, 0.2, 0.8));
                glBegin(GL_LINES);
                glVertex3f(v[0], v[1], v[2]);
                glVertex3f(v[0] + f[0], v[1] + f[1], v[2] + f[2]);
                glEnd();
                mRI->pushMatrix();
                glTranslated(v[0], v[1], v[2]);
                mRI->drawEllipsoid(Vector3d(0.02, 0.02, 0.02));
                mRI->popMatrix();
            }
        }
    }

    for (unsigned int i = 0; i < mSkels.size(); i++)
        mSkels[i]->draw(mRI);
        
    // display the frame count in 2D text
    char buff[64];
    if (!mSim) 
        sprintf(buff, "%d", mPlayFrame);
    else{
        sprintf(buff, "%d", mSimFrame);
    }
    string frame(buff);
    glDisable(GL_LIGHTING);
    glColor3f(0.0,0.0,0.0);
    yui::drawStringOnScreen(0.02f,0.02f,frame);
    glEnable(GL_LIGHTING);
}

void MyWindow::keyboard(unsigned char key, int x, int y)
{
    ifstream inFile;
    ofstream outFile("output.txt", ios::out);
    ofstream dofFile("simMotion.dof", ios::out);
    char buffer[80];
    int nFrames;
    Vector3d val;
    switch(key){
    case ' ': // use space key to play or stop the motion
        mSim = !mSim;
        if (mSim) {
            mPlay = false;
            glutTimerFunc( mDisplayTimeout, refreshTimer, 0);
        }
        break;
    case 's': // simulate one frame
        if (!mPlay) {
            mForce = Vector3d::Zero();
            setPose();
            mIntegrator.integrate(this, mTimeStep);
            mSimFrame++;
            bake();
            glutPostRedisplay();
        }
        break;
    case '1': // upper right force
        mForce[0] = 50;
        //        mForce[2] = 75;
        mImpulseDuration = 10.0;
        cout << "push forward right" << endl;
        break;
    case '2': // upper right force
        mForce[0] = -50;
        mImpulseDuration = 10.0;
        cout << "push backward" << endl;
        break;
    case 'p': // playBack
        mPlay = !mPlay;
        if (mPlay) {
            mSim = false;
            glutTimerFunc( mDisplayTimeout, refreshTimer, 0);
        }
        break;
    case '[': // step backward
        if (!mSim) {
            mPlayFrame--;
            if(mPlayFrame < 0)
                mPlayFrame = 0;
            glutPostRedisplay();
        }
        break;
    case ']': // step forwardward
        if (!mSim) {
            mPlayFrame++;
            if(mPlayFrame >= mBakedStates.size())
                mPlayFrame = 0;
            glutPostRedisplay();
        }
        break;
    case 'v': // show or hide markers
        mShowMarkers = !mShowMarkers;
        break;
        
    case 'a': // save deisred momentum
        outFile.precision(10);
        outFile << mController->mLinHist.size() << endl;
        for (int i = 0; i < mController->mLinHist.size(); i++) {
            outFile << i << endl;
            outFile << mController->mLinHist[i][0] << " " << mController->mLinHist[i][1] << " " << mController->mLinHist[i][2] << " "<< mController->mAngHist[i][0] << " " << mController->mAngHist[i][1] << " " << mController->mAngHist[i][2] << endl;
        }
        break;
        
    case 'l': // load desired momentum
        inFile.open("launch.txt");
        inFile.precision(10);
        inFile >> buffer; // nFrames
        nFrames = atoi(buffer);
        for (int i = 0; i < nFrames; i++) {
            inFile >> buffer; // discard the frame number
            for (int j = 0; j < 3; j++) {
                inFile >> buffer;
                val[j] = atof(buffer);
            }
            mController->mLaunchLin.push_back(val);
            for (int j = 0; j < 3; j++) {
                inFile >> buffer;
                val[j] = atof(buffer);
            }
            mController->mLaunchAng.push_back(val);
        }
        cout << "load " << mController->mLaunchAng.size() << " frames from launch.txt" << endl;
        inFile.close();
        
        inFile.open("landing.txt");
        inFile.precision(10);
        inFile >> buffer; // nFrames
        nFrames = atoi(buffer);
        for (int i = 0; i < nFrames; i++) {
            inFile >> buffer; // discard the frame number
            for (int j = 0; j < 3; j++) {
                inFile >> buffer;
                val[j] = atof(buffer);
            }
            mController->mLandingLin.push_back(val);
            for (int j = 0; j < 3; j++) {
                inFile >> buffer;
                val[j] = atof(buffer);
            }
            mController->mLandingAng.push_back(val);
        }
        cout << "load " << mController->mLandingAng.size() << " frames from landing.txt" << endl;
        inFile.close();        
        break;

    case 'z': // save current dof motion
        dofFile.precision(10);
        dofFile << "frame = " << mBakedStates.size() << " dofs = " << mSkels[1]->getNumDofs() << endl;
        for (int i = 0; i < mSkels[1]->getNumDofs(); i++) 
            dofFile << mSkels[1]->getDof(i)->getName() << " ";
        dofFile << endl;
        for (int i = 0; i < mBakedStates.size(); i++) {
            for (int j = 0; j < mSkels[1]->getNumDofs(); j++)
                dofFile << mBakedStates[i][mIndices[1] + j] << " ";
            dofFile << endl;
        }
        break;

    default:
        Win3D::keyboard(key,x,y);
    }
    glutPostRedisplay();
}

void MyWindow::bake()
{
    int nContact = mCollisionHandle->getCollisionChecker()->getNumContact();
    VectorXd state(mIndices.back() + 6 * nContact);
    for (unsigned int i = 0; i < mSkels.size(); i++)
        state.segment(mIndices[i], mDofs[i].size()) = mDofs[i];
    for (int i = 0; i < nContact; i++) {
        int begin = mIndices.back() + i * 6;
        state.segment(begin, 3) = mCollisionHandle->getCollisionChecker()->getContact(i).point;
        state.segment(begin + 3, 3) = mCollisionHandle->getCollisionChecker()->getContact(i).force;
    }

    mBakedStates.push_back(state);
}

Vector3d MyWindow::evalLinMomentum(SkeletonDynamics* _skel, const VectorXd& _dofVel) {
    MatrixXd J(MatrixXd::Zero(3, _skel->getNumDofs()));
    for (int i = 0; i < _skel->getNumNodes(); i++) {
        BodyNodeDynamics *node = (BodyNodeDynamics*)_skel->getNode(i);
        MatrixXd localJ = node->getJacobianLinear() * node->getMass();
        for (int j = 0; j < node->getNumDependentDofs(); j++) {
            int index = node->getDependentDof(j);
            J.col(index) += localJ.col(j);
        }
    }
    Vector3d cDot = J * _dofVel;
    return cDot / mSkels[1]->getMass();
}

Vector3d MyWindow::evalAngMomentum(SkeletonDynamics* _skel, const VectorXd& _dofVel) {
    Vector3d c = _skel->getWorldCOM();
    Vector3d sum = Vector3d::Zero();
    Vector3d temp = Vector3d::Zero();
    for (int i = 0; i < _skel->getNumNodes(); i++) {
        BodyNodeDynamics *node = (BodyNodeDynamics*)_skel->getNode(i);
        node->evalVelocity(_dofVel);
        node->evalOmega(_dofVel);
        sum += node->getInertia() * node->mOmega;
        sum += node->getMass() * (node->getWorldCOM() - c).cross(node->mVel);
    }
    return sum;
}

void MyWindow::sampleControl() {
    mBestTorques = mController->getTorques();
    mBestScore = 1e7;
    mBestAlpha = 0;
    VectorXd sample(mBestTorques.size());
    sample.setZero();
    // set to original control in case all samples are worse
    mSkels[1]->setInternalForces(mController->getTorques());
    pushState();
    mIntegrator.integrate(this, mTimeStep);
    evaluateSample(sample); // so we can update mBestScore for the original control
    popState();

    /* // sampling virtual force to meet desired linear momentum
    for (int i = 0; i < 30; i++) {
        double alpha = -30 + i * 2;
        Vector3d vForce(0, 100, 0);
        vForce *= alpha;
        sample = mController->computeVirtualTorque(vForce);
        mSkels[1]->setInternalForces(mController->getTorques() + sample);
        pushState();
        mIntegrator.integrate(this, mTimeStep);
        if (evaluateSample(sample))
            mBestAlpha = alpha;
        popState();        
    }*/
    
    // sampilng controlBias to meet desired angular momentum
    for (int i = 0; i < 10; i++) {
        double alpha = -10 + i * 2;
        sample = alpha * mControlBias;
        // we can compare with stupid random sampling
        //for (int j = 0; j < sample.size(); j++)
        //  sample[j] = utils::random(-40, 40);
        mSkels[1]->setInternalForces(mController->getTorques() + sample);
        pushState();
        mIntegrator.integrate(this, mTimeStep);
        if (evaluateSample(sample))
            mBestAlpha = alpha;
        popState();
    }
}

bool MyWindow::evaluateSample(VectorXd& _sample) {
    Vector3d lin = evalLinMomentum(mSkels[1], mDofVels[1]);
    Vector3d ang = evalAngMomentum(mSkels[1], mDofVels[1]);
    double sampleVal = ang[2];
    double target = mController->mLaunchAng[mController->mPrepareFrame][2] * 2.0;
    if (mBestScore > abs(sampleVal - target)) {
        mBestScore = abs(sampleVal - target);
        mBestTorques = mController->getTorques() + _sample;
        return true;
    }
    return false;
}

void MyWindow::pushState() {
    for (int i = 0; i < mDofs.size(); i++) 
        mStoredDofs[i] = mDofs[i];

    for (int i = 0; i < mDofVels.size(); i++) 
        mStoredDofVels[i] = mDofVels[i];
}

void MyWindow::popState() {
    for (int i = 0; i < mDofs.size(); i++) 
        mDofs[i] = mStoredDofs[i];

    for (int i = 0; i < mDofVels.size(); i++) 
        mDofVels[i] = mStoredDofVels[i];
}

/*
// formulation for generalized coordintates
Vector3d MyWindow::evalAngMomentum(const VectorXd& _dofVel) {
    Vector3d ret = Vector3d::Zero();
    Vector3d c = mSkels[1]->getWorldCOM();
    for (int i = 0; i < mSkels[1]->getNumNodes(); i++) {
        BodyNodeDynamics *node = (BodyNodeDynamics*)mSkels[1]->getNode(i);
        Vector3d localC = node->getLocalCOM();
        Matrix4d W = node->getWorldTransform();
        Matrix4d localTranslate = Matrix4d::Identity(4, 4);
        localTranslate.block(0, 3, 3, 1) = localC;
        Matrix4d M = localTranslate * node->getShape()->getMassTensor() * localTranslate.transpose();
        Matrix4d WDot = Matrix4d::Zero();
        for (int j = 0; j < node->getNumDependentDofs(); j++) {
            int index = node->getDependentDof(j);
            WDot += node->getDerivWorldTransform(j) * _dofVel[index];
        }
        Matrix3d R = W.block(0, 0, 3, 3);
        Matrix3d RDot = WDot.block(0, 0, 3, 3);
        Vector3d r = W.block(0, 3, 3, 1);
        Vector3d rDot = WDot.block(0, 3, 3, 1);

        Matrix3d rotation = R * M.block(0, 0, 3, 3) * RDot.transpose();
        Matrix3d translation = node->getMass() * r * rDot.transpose();
        Matrix3d crossTerm1 = node->getMass() * r * localC.transpose() * RDot.transpose();
        Matrix3d crossTerm2 = node->getMass() * R * localC * rDot.transpose();
        ret += utils::crossOperator(rotation + translation + crossTerm1 + crossTerm2);

        ret -= node->getMass() * c.cross(utils::xformHom(WDot, localC));
    }
    return ret;
}
*/
