/////////////////////////////////////////////////////////////////////////////////////////
// Create scene

var scene = new THREE.Scene();
var renderer = new THREE.WebGLRenderer({ canvas: renderCanvas });
var camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);

renderer.setSize(window.innerWidth, window.innerHeight);

/////////////////////////////////////////////////////////////////////////////////////////
// Pause control

var paused = false;
document.addEventListener("keypress", function(event) 
{
    if (event.key == ' ')
    {
        if (paused)
            paused = false;
        else
            paused = true;
    }    
});

/////////////////////////////////////////////////////////////////////////////////////////
// Commonly used colours

var GREEN = 0x00FF00;
var RED = 0xFF0000;
var YELLOW = 0xFFFF33;
var CYAN = 0x00FFFF;
var ORANGE = 0xFFA500;
var WHITE = 0xFFFFFF;

/////////////////////////////////////////////////////////////////////////////////////////
// Coordinate sytem

class CoordinateSystem
{
    static addToScene()
    {
        scene.add(CoordinateSystem.xAxis);
        scene.add(CoordinateSystem.yAxis);
        scene.add(CoordinateSystem.zAxis);
    }

    static removeFromScene()
    {
        scene.remove(CoordinateSystem.xAxis);
        scene.remove(CoordinateSystem.yAxis);
        scene.remove(CoordinateSystem.zAxis);
    }
}

CoordinateSystem.xAxis = new THREE.ArrowHelper(new THREE.Vector3( 1, 0, 0 ), new THREE.Vector3(0, 0, 0), 1, GREEN );
CoordinateSystem.yAxis = new THREE.ArrowHelper(new THREE.Vector3( 0, 1, 0 ), new THREE.Vector3(0, 0, 0), 1, YELLOW );
CoordinateSystem.zAxis = new THREE.ArrowHelper(new THREE.Vector3( 0, 0, 1 ), new THREE.Vector3(0, 0, 0), 1, RED );
CoordinateSystem.addToScene();

/////////////////////////////////////////////////////////////////////////////////////////
// Render Loop

var renderLoop = function()
{
    requestAnimationFrame(renderLoop);
    renderer.render(scene, camera);
}

renderLoop();