class HistoryGraph
{
    constructor(x, y, w, h, halfScaleX, halfScaleY, maxCount = 5)
    {
        this.history = [];
        this.maxCount = maxCount;
        this.total = 0;
        this.dots = [];

        this.halfScaleX = halfScaleX;
        this.halfScaleY = halfScaleY;

        this.createGraph(x, y, w, h);
        this.drawAxes();
    }

    createGraph(x, y, w, h)
    {
        this.graphCanvas = document.createElement('canvas');
        this.graphCanvas.setAttribute('class', 'graphCanvas');
        document.getElementsByClassName('stack')[0].appendChild(this.graphCanvas);

        this.graphCanvas.style.left = x;
        this.graphCanvas.style.top = y;
        this.graphCanvas.style.width = w;
        this.graphCanvas.style.height = h;
    }

    drawAxes()
    {
        const ctx = this.graphCanvas.getContext('2d');
        ctx.strokeStyle = "#FFFFFF";

        ctx.beginPath();
        this.middleX = (this.graphCanvas.width / 2);
        this.middleY = this.graphCanvas.height / 2;
        ctx.moveTo(this.middleX,0);
        // End point (180,47)
        ctx.lineTo(this.middleX,this.graphCanvas.height);
        // Make the line visible
        ctx.moveTo(0,this.middleY);
        ctx.lineTo(this.graphCanvas.width,this.middleY);

        ctx.stroke();
    }

    xValueToPixel(x)
    {
        return this.middleX + Math.round(x * (this.middleX / this.halfScaleX));
    }

    yValueToPixel(y)
    {
        return this.middleY - Math.round(y * (this.middleY / this.halfScaleY));
    }

    drawPoint(x, y)
    {
        var ctx = this.graphCanvas.getContext("2d");
        ctx.beginPath();
        ctx.arc(x, y, 5, 0, 2 * Math.PI);
        ctx.fillStyle = 'white';
        ctx.fill();
    }

    reset()
    {
        const ctx = this.graphCanvas.getContext('2d');
        ctx.clearRect(0, 0, this.graphCanvas.width, this.graphCanvas.height);
    }

    record(x, y)
    {
        let index = this.total;
        let value = { x: this.xValueToPixel(x), y: this.yValueToPixel(y) };

        if (this.history.length >= this.maxCount)
        {
            index = this.total % this.maxCount;
            this.history[index] = value;
        }
        else
        {
            this.history.push(value);
            this.dots.push()
        }

        this.total += 1;
    }

    draw()
    {
        this.reset();
        this.drawAxes();

        for (let i = 0; i < this.history.length; ++i)
        {
            this.drawPoint(this.history[i].x, this.history[i].y);
        }
    }
}

