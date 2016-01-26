

// Un genre de carré de light 
p._position = glm::vec4( 
                    fd+ 5 * glm::mod((float)i, float(sqrt(nbPointLights)))
                    ,0.01
                    ,fd+ 5 * i / int(sqrt(nbPointLights))
                    ,1.0);

// Un cercle de light
p._position = glm::vec4( 
                    fd+ 5 * cos(i*2*3.14/nbPointLights)
                    ,0.01
                    ,fd+ 5 * sin(i*2*3.14/nbPointLights)
                    ,1.0);

// Un cercle avec des point lights qui tournent autour du cercle
p._position = glm::vec4( 
                    fd+ 5 * cos(i+t*2* M_PI /nbPointLights)
                    ,0.01
                    ,fd+ 5 * sin(i+t*2* M_PI /nbPointLights)
                    ,1.0);


// ...
 int fd = 35; // first decal
            // std::vector<int> rayons = {1, 6, 12};
            int rayons[] = {5,15,25,35,45,55,65,75,85,95,105, 115, 125};
            int nbLightsByCircle[] = {6, 10, 15, 21, 28, 36, 45, 55, 61, 73, 86, 100, 115};
            // int nbLightsByCircle[] = {6, 12, 24, 48};
            
            int rayon = 5;

            for(int i = 0; i < nbPointLights; ++i){

                // i++;
                PointLight p;
                // p._position = glm::vec4( (nbPointLights*cosf(t)) * sinf(t*i), 1.0, fabsf(nbPointLights*sinf(t)) * cosf(t*i), 1.0 );
                p._color = glm::vec3(fabsf(cos(t+i*2.f)), 1.-fabsf(sinf(t+i)) , 0.5f + 0.5f-fabsf(cosf(t+i)) );

                if( i == nbLightsByCircle[counterCircle] ){
                  counterCircle++;
                  rayon += 10;  
                } 
                float coeff = rayon;
                float rotate = t + t/2;
                p._position = glm::vec4( 
                    fd+ coeff * cos(i+rotate*2* M_PI /nbPointLights)  
                    ,0.01
                    ,fd+ coeff * sin(i+rotate*2* M_PI /nbPointLights) 
                    ,1.0);
                // p._color = glm::vec3(0.1,0.1,0.95);

                p._intensity = 0.5;

                glProgramUniformMatrix4fv(pointLightProgramObject, pointLightScreenToWorldLocation, 1, 0, glm::value_ptr(screenToWorld));
                glProgramUniform1f(pointLightProgramObject, pointLightIntensityLocation, p._intensity);
                glProgramUniform3fv(pointLightProgramObject, pointLightPositionLocation, 1, glm::value_ptr(glm::vec3(p._position) / p._position.w));
                glProgramUniform3fv(pointLightProgramObject, pointlightColorLocation, 1, glm::value_ptr(glm::vec3(p._color)));
                glProgramUniform3fv(pointLightProgramObject, pointLightCameraPositionLocation, 1, glm::value_ptr(camera.eye));
                glProgramUniform1f(pointLightProgramObject, pointlightTimeLocation, t);
                glProgramUniform1f(pointLightProgramObject, pointlightCounterLocation, (int)nbPointLights);

                // glProgramUniformMatrix4fv(pointLightProgramObject, pointLightScreenToWorldLocation, 1, 0, glm::value_ptr(screenToWorld));
                // glProgramUniform1f(pointLightProgramObject, pointLightIntensityLocation, pointLights[i]._intensity);
                // glProgramUniform3fv(pointLightProgramObject, pointLightPositionLocation, 1, glm::value_ptr(glm::vec3(pointLights[i]._position) / pointLights[i]._position.w));
                // glProgramUniform3fv(pointLightProgramObject, pointlightColorLocation, 1, glm::value_ptr(glm::vec3(pointLights[i]._color)));
                // glProgramUniform3fv(pointLightProgramObject, pointLightCameraPositionLocation, 1, glm::value_ptr(camera.eye));
                // glProgramUniform1f(pointLightProgramObject, pointlightTimeLocation, t);
                // glProgramUniform1f(pointLightProgramObject, pointlightCounterLocation, nbPointLights);

              
                // Render quad
                glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
            }