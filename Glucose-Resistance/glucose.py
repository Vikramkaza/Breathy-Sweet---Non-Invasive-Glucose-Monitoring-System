#Glucose prediction

#Importing the libraries

import numpy as np

import pandas as pd

#Importing the dataset

def prediction(num):
    
    dataset = pd.read_csv('Glucose-Resistance.csv')
    X = dataset.iloc[1:, 1:2].values
    y = dataset.iloc[1:, 0].values
    
    # Training the Decision Tree Regression model on the whole dataset
    
    from sklearn.tree import DecisionTreeRegressor
    regressor = DecisionTreeRegressor(random_state = 0)
    regressor.fit(X, y)
    
    #Predicting a new result
    
    print(regressor.predict([[num]]))
    y=regressor.predict([[num]])
    return y
    
    
