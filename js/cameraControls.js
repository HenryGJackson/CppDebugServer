var pi = 3.1415;

////////////////////////////////////////////////////////////////////////
// Adds the ability to move the camera with the mouse

class MouseTracker
{
    constructor() 
    {
        this.x = 0;
        this.y = 0;
        this.capturing = false;
    }
    beginCapture()
    {
        this.capturing = true;
    }
    capture(x, y)
    {
        this.x = x;
        this.y = y; 
    }
    deltaX(x)
    {
        return x - this.x;
    }
    deltaY(y)
    {
        return y - this.y;
    }
    release()
    {
        this.capturing = false;
    }
}

////////////////////////////////////////////////////////////////////////

var degreesToRadians = function(degrees)
{
    return degrees * pi / 180
}

////////////////////////////////////////////////////////////////////////

class CameraController
{
    constructor() 
    {
        camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
        this.hFOV = 2 * Math.atan(Math.tan(camera.fov * Math.PI / 360) * camera.aspect); // degrees
        this.vFOV = camera.fov;

        this.theta = pi * 0.25;
        this.phi = pi * 0.25;

        this.cameraDistance = 10;
        this.moveSpeed = 0.5;
        this.lookDirection = new THREE.Vector3(0, 0, -1);

        mouseTracker.beginCapture();
        this.update(0, 0);
        mouseTracker.release();
    }

    update(mouseX, mouseY)
    {
        if (!mouseTracker.capturing)
            return;

        let dx = mouseTracker.deltaX(mouseX);
        let dy = mouseTracker.deltaY(mouseY);
        mouseTracker.capture(mouseX, mouseY);

        this.phi += degreesToRadians(dx * this.moveSpeed * 0.5); 
        this.theta -= degreesToRadians(dy * this.moveSpeed * 0.5); 
        this.theta = Math.min(this.theta, pi);
        this.theta = Math.max(0.0001, this.theta);
        // console.log(`theta: ${this.theta}, phi: ${this.phi}`);

        this.updatePosition();
    }

    move(forwards)
    {
        if (forwards)
        {
            if (cameraController.cameraDistance < 1)
                return;
            
            cameraController.cameraDistance -= cameraController.moveSpeed;
        }
        else
        {
            if (cameraController.cameraDistance > 100)
                return;
            
            cameraController.cameraDistance += cameraController.moveSpeed;
        }

        cameraController.updatePosition();
    }

    updatePosition()
    {
        let sinTheta = Math.sin(this.theta);
        camera.position.y = this.cameraDistance * Math.cos(this.theta);
        camera.position.z = this.cameraDistance * sinTheta * Math.sin(this.phi);
        camera.position.x = this.cameraDistance * sinTheta * Math.cos(this.phi);
        
        if (sinTheta = 0 && phi > PI)
        {
            camera.position.y = - camera.position.y;
        }

        camera.lookAt(0, 0, 0);
    }

    getLookDirection()
    {
        camera.getWorldDirection(this.lookDirection);
        return this.lookDirection;
    }
}

////////////////////////////////////////////////////////////////////////

var mouseTracker = new MouseTracker();
var cameraController = new CameraController();

document.addEventListener('mousedown', function(event) 
{
    mouseTracker.beginCapture();
    mouseTracker.capture(event.pageX, event.pageY);
});

document.addEventListener('mouseup', function(event)
{
    mouseTracker.release();
});

document.addEventListener('mousemove', function(event)
{
    cameraController.update(event.pageX, event.pageY);
});

document.addEventListener('wheel', function(event)
{
    if (event.deltaY < 0)
        cameraController.move(true);
    else
        cameraController.move(false);


    // if (event.key == 'w')
    //     cameraController.move(true);

    // else if (event.key == 's')
    //     cameraController.move(false);

    // cameraController.moveSpeed -= event.deltaY * 0.001;
    // console.log(`move speed changed to: ${cameraController.moveSpeed}`);
});

document.addEventListener("keypress", function(event) 
{
    if (event.key == 'w')
        cameraController.moveSpeed += 0.1;

    else if (event.key == 's')
        cameraController.moveSpeed -= 0.1;

    else if (event.code == 'm')
        console.log("ALT");


    console.log(event.code);
});