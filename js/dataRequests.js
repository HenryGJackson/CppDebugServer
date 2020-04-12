

class Client
{
    static requestData(dataId, callback, dataConversionCallback)
    {
        let xmlHttp = new XMLHttpRequest();
        xmlHttp.onreadystatechange = function()
        {
            // readyState == 4 means the request is complete
            // status == 200 means the request is loading or done.
            if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
            {
                let data = xmlHttp.responseText.split(",");
                data.forEach(function(value, index, array) 
                { 
                    array[index] = dataConversionCallback(value); 
                });
                callback(data);
            }    
        }

        xmlHttp.open("GET", "data/" + dataId, true);
        xmlHttp.send(null);
    }

    static requestFloatData(dataId, callback)
    {
        Client.requestData(dataId, callback, parseFloat);
    }

    static requestIntData(dataId, callback)
    {
        Client.requestData(dataId, callback, parseInt);
    }

    static makeRequests()
    {
        for (let i = 0; i < Client.requesters.length; ++i)
        {
            Client.requesters[i].request();
        }
    }
}

Client.requesters = [];

class DataRequester
{
    constructor(dataId, callback)
    {
        this.dataId = dataId;
        this.callback = callback;
        this.canExecuteNextRquest = true;
        Client.requesters.add(this);
    }

    request()
    {
        if (!this.canExecuteNextRquest)
            return;

        this.canExecuteNextRquest = false;
        let requester = this;
        Client.requestFloatData(this.dataId, function(data) 
        {
            requester.callback(data);
            requester.canExecuteNextRquest = true;
        });
    }
}

var requestsLoop = function()
{
    requestAnimationFrame(requestsLoop);
    Client.makeRequests();
}

requestsLoop();