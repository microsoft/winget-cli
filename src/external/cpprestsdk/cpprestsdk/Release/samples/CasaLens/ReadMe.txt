========================================================================
    CasaLens Project Overview
========================================================================

CasaLens is a data mash-up sample i.e use the Casablanca features to build a service that can collate data from different services and provide them to the user. 
Given the postal code/city name, the service overlays events, movies, weather, pictures around the place.

To run the sample, obtain the api keys from the different services (listed below) and update them in casalens.cpp (see Services section below for more details).
The sample takes one parameter: the port to listen for requests.

Services:
Here are the details about the services the sample is interacting with, how to obtain the api keys for using these services and how to pass it to the sample. 
1. Bing image search: http://datamarket.azure.com/dataset/bing/search
   Sign UP at the above link to obtain the api key.
   In casalens.cpp, set the value of bmaps_key variable to this key.
   Query this service for pictures of a place and return it to the client.
   This service is also used to collect movie posters.

2. OpenWeatherMap:http://openweathermap.org/ 
   Fetch weather data: current temperature, pressure at the location.

3. Eventful: http://http://api.eventful.com/
   Follow the "Get Started" steps -> "Register a new account" at the eventful website to obtain the application key.
   In casalens.cpp, set the value of events_key variable to this key.
   Fetch different events happening at the specified location.

4. OnConnect tmsapi: http://developer.tmsapi.com/
   Register at the above website and obtain a new key for the TMS API package.
   In casalens.cpp, set the value of movies_key variable to this key.
   Get currently playing movies in local theaters along with the show times.
   
5. Google maps: https://developers.google.com/maps
   This does not require a key.
   Given a postal code, use google maps API to get the location (city name) corresponding to that code.

6. Bing maps: http://dev.virtualearth.net/
   The key populated in step 1 should work for both bing maps and search.
   Given a location (city name), use bing maps API to get the postal code.

Files:
1. casalens.cpp: 
   Main file that contains code to initialize and start the http_listener.
   We add two GET and POST handlers handle_get and handle_post and open the listener to listen for requests.

2. datafetcher.cpp:
   This file contains logic to collect data from different services, create JSON objects with the data and return it to the client.

This sample is merely a demonstration of how one can use Casablanca to author data mash-ups. 
If you plan to use/deploy the sample, do not forget to read and follow the "Terms and Conditions" for each service and ensure that you are adhering to all the requirements.
