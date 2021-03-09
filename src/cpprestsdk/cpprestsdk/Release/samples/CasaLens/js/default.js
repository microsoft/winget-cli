function formatAMPM(date) {
    var hours = date.getHours();
    var minutes = date.getMinutes();
    var ampm = hours >= 12 ? 'pm' : 'am';
    hours = hours % 12;
    hours = hours ? hours : 12; // the hour '0' should be '12'
    minutes = minutes < 10 ? '0' + minutes : minutes;
    var strTime = hours + ':' + minutes + ' ' + ampm;
    return strTime;
}

function formatEventTime(dateTimeStr) {
    dateTimeStr = dateTimeStr.substr(0, dateTimeStr.lastIndexOf(":")).split(" ").join("T")
    var dateObj = new Date(dateTimeStr);
    if (!dateObj) {
        return "";
    }
    var dateStr = dateObj.toDateString();
    dateStr = dateStr.substring(0, dateStr.length - 5);
    return dateStr + "<br/>" + formatAMPM(dateObj);
}

function ClearText() {
    location_ip.value = "";
}

function TrimText(text, maxLength) {
    if (text.length < maxLength) {
        return text;
    } else {
        var i = maxLength;
        while (i < text.length) {

            if (text[i] == " " || text[i] == "," || text[i] == "." || text[i] == "\r" || text[i] == "\n") {
                break;
            }
            i++;
        }
        return text.substr(0, i);
    }
}

function populateEvents(events) {

    var tempDiv = document.createElement("div");

    var currentChildNodes = eventsTable.childNodes;
    var defaultChildrenCount = 2;
    for (var i = defaultChildrenCount; i < currentChildNodes.length; i++) {
        currentChildNodes[i].parentNode.removeChild(currentChildNodes[i]);
    }
    for (var i = 0; i < events.length; i++) {
        var dataObj = events[i];

        tempDiv.innerHTML = dataObj.description;
        var description = TrimText(tempDiv.innerText, 150);
        var innerHtml = eventTemplate.innerHTML;
        innerHtml = innerHtml.replace("%eventtitle%", TrimText(dataObj.title, 30));
        innerHtml = innerHtml.replace("%eventstarttime%", formatEventTime(dataObj.starttime));
        innerHtml = innerHtml.replace("%eventdescription%", description);
        innerHtml = innerHtml.replace("%eventlink1%", dataObj.url);
        innerHtml = innerHtml.replace("%eventlink2%", dataObj.url);
        innerHtml = innerHtml.replace("%eventvenue%", dataObj.venue_address);
        var obj = document.createElement("tr");
        obj.innerHTML = window.toStaticHTML(innerHtml);

        eventsTable.appendChild(obj);
    }
}

function populateWeather(weather) {

    var currentChildNodes = weatherData.childNodes;
    var defaultChildrenCount = 5;
    for (var i = defaultChildrenCount; i < currentChildNodes.length; i++) {
        currentChildNodes[i].parentNode.removeChild(currentChildNodes[i]);
    }

    var weatherElement = weatherTemplate.cloneNode(true);
    weatherElement.style.display = "inherit";
    var innerHtml = weatherElement.innerHTML;
    innerHtml = innerHtml.replace("%wt%", weather.temperature);
    innerHtml = innerHtml.replace("%wp%", weather.pressure);
    innerHtml = innerHtml.replace("%wtmin%", weather.temp_min);
    innerHtml = innerHtml.replace("%wtmax%", weather.temp_max);
    innerHtml = innerHtml.replace("%wdesc%", weather.description);
    innerHtml = innerHtml.replace("%wimg%", weather.image);
    weatherElement.innerHTML = window.toStaticHTML(innerHtml);
    weatherData.appendChild(weatherElement);
}

function populateMovies(movies) {

    var currentChildNodes = movieTemplateToHide.childNodes;
    var defaultChildrenCount = 3;
    for (var i = defaultChildrenCount; i < currentChildNodes.length; i++) {
        currentChildNodes[i].parentNode.removeChild(currentChildNodes[i]);
    }

    var showtimes = "";
    for (var i = 0; i < movies.length; i++) {
        var innerHtml = movieTemplate.innerHTML;
        var dataObj = movies[i];

        var theatrename;
        for (var j = 0; j < dataObj.theatre.length; j++) {

            showtimes += dataObj.theatre[j].name;
            showtimes += "<br/>";
            var showtimelist = dataObj.theatre[j].datetime;
            for (var k = 0; k < showtimelist.length; k++) {
                var dateObj = new Date(showtimelist[k]);
                if (dateObj) {
                    showtimes += formatAMPM(dateObj) + " | ";
                }
            }
            showtimes = showtimes.substr(0, showtimes.length - 2);
            showtimes += "<br/><br/>";
        }
        innerHtml = innerHtml.replace("%mtitle%", dataObj.title);
        innerHtml = innerHtml.replace("%mtheater%", showtimes);
        innerHtml = innerHtml.replace("%mposter%", dataObj.poster);

        var obj = document.createElement("div");
        obj.innerHTML = window.toStaticHTML(innerHtml);
        movieTemplateToHide.appendChild(obj);
    }
}

function populateImages(images) {
    if (images.length == 4) {
        var mainimg = document.getElementById('mainPic1');
        mainimg.src = images[0];

        var img1 = document.getElementById('thumbnailPic1');
        img1.src = images[1];

        var img2 = document.getElementById('thumbnailPic2');
        img2.src = images[2];

        var img3 = document.getElementById('thumbnailPic3');
        img3.src = images[3];
    }
}

function FetchData() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function (e) {
        // The request finished and response is ready
        if (xhr.readyState == 4) {
            var searchbar = document.getElementById('searchBar');
            searchbar.style.display = 'none';

            if (xhr.status == 200) {
                var replydata = JSON.parse(xhr.responseText);
                populateImages(replydata.images);
                populateEvents(replydata.events);
                populateMovies(replydata.movies);
                populateWeather(replydata.weather);

                defaultData.style.display = "none";
                cityData.style.display = 'inherit';
            }
            else {
                defaultData.style.display = "none";
                errorData.style.display = 'inherit';
            }
        }
    }
    xhr.open("POST", document.URL, true);
    xhr.setRequestHeader("Content-type", 'text/html');
    xhr.send(location_ip.value);
}
