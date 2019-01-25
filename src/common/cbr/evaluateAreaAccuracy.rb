#!/usr/bin/ruby


#
# Copyright (C) 2009 Institut fuer Telematik, Universitaet Karlsruhe (TH)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# == Synopsis
#
# Evaluate Area Accuracy : Evaluate an area-map against a reference area-map and calculate an accuracy index
#
# Optional Parameters: see below
#
# == Usage
#
# evaluateAreaAccuracy.rb [OPTION] ... gvbDataBaseDirectory strategyName nodecount parameter
# 
# eg.: ruby evaluateAreaAccuracy.rb -p 4 ../../../simulations/gvbData/ simplifyCoords 2000 "10,50,100,250,500,750,1000,2500,5000,10000,50000,100000"
#
# -h, --help: 
#   show this help
# -p x, --maxprefix x:
#   maximum prefix length in bits (default: 4)
# -n x, --nodexount x:
#   overlay node count
#
# gvbDataBaseDirectory: path to the base directory
# strategyName: Name of the Strategy, e.g. simplifyCoords
# parameter: Strategy parameter: limitCoords,10

########################### Requirements ################################

require 'getoptlong'
require 'xml/libxml'       # requires libxml-ruby
require 'rdoc/usage'

require 'rubygems'
require 'faster_csv'



########################### CommandLineStuff ################################

opts = GetoptLong.new(
    [ '--help', '-h', GetoptLong::NO_ARGUMENT ],
    [ '--maxprefix', '-p', GetoptLong::REQUIRED_ARGUMENT ]
)

maxprefix = 4

opts.each do |opt, arg|
    case opt
        when '--help'
            RDoc::usage
        when '--maxprefix'
            maxprefix = arg.to_i
    end
end



unless ARGV.length == 4
    puts "Missing file argument (try --help)"
    exit 0
end

########################### Default Values ################################
gvbDataBaseDirectory = nil
strategyName = nil
parameter = nil
nodecount = 10

########################### Data from Command Line ################################
gvbDataBaseDirectory = ARGV.shift
strategyName = ARGV.shift
nodecount = ARGV.shift
parameter = ARGV.shift
########################### AreaEvaluate Class ############################

###
# Class EvaluationConfigCollection
# Builds Config Objects for all Parameter Combinations
###
class EvaluationConfigCollection
  
  def initialize()
    @configCollection = Array.new
    @additionalParameters = Hash.new
  end
  
  def createCollection(gvbDataBaseDirectory, strategyName, nodecount, parameter, maxPrefix)
    
    @configSourceArray = Hash.new
    
    
    @configSourceArray['strategy'] = getMultipleParameterAsArray(strategyName)
    @configSourceArray['nodecount'] = getMultipleParameterAsArray(nodecount)
    @configSourceArray['parameter'] = Array.new
    paramArray = parameter.split(';')
    
    paramArray.each do |param|

      @configSourceArray['parameter'] << getMultipleParameterAsArray(param)

    end
    
    #TODO Make it cute!!!!      
    @configSourceArray['strategy'].each do |itStrategy|
        @configSourceArray['nodecount'].each do |itNodecount|
          @configSourceArray['parameter'][0].each do |itParameter0|
            
            if  @configSourceArray['parameter'][1] == nil
              @configCollection << EvaluationConfig.new(gvbDataBaseDirectory, itStrategy, itNodecount, itParameter0, maxPrefix)    
            else
              
              @configSourceArray['parameter'][1].each do |itParameter1|
                if  @configSourceArray['parameter'][2] == nil
                  @configCollection << EvaluationConfig.new(gvbDataBaseDirectory, itStrategy, itNodecount, itParameter0 + ';' + itParameter1, maxPrefix)
                else
                  
                  @configSourceArray['parameter'][2].each do |itParameter2|
                    @configCollection << EvaluationConfig.new(gvbDataBaseDirectory, itStrategy, itNodecount, itParameter0 + ';' + itParameter1 + ';' + itParameter2, maxPrefix)
                  end
                end
              end
            end
          end
        end
    end
    
    return @configCollection
          
  end
  
  
  
  def hasMultipleParameters(parameter)
    parameter = parameter.to_s
    return  parameter.count(',') > 0 ? true : false
  end
  
  def getMultipleParameterAsArray(parameter)
    
    if(hasMultipleParameters(parameter))
      return parameter.split(',')
      
    else
      return Array.new(1, parameter)
    end
    
  end
  
end

###
# Class EvaluationConfig
# Holds the Config and build paths
###
class EvaluationConfig

  def initialize(gvbDataBaseDirectory, strategyName, nodecount, parameter, maxPrefix)
    
    @gvbDataBaseDirectory = gvbDataBaseDirectory[-1,1] == '/' ? gvbDataBaseDirectory : gvbDataBaseDirectory + '/'
    @strategyName = strategyName
    @parameter = parameter.split(';')
    
    @nodecount = nodecount.to_s;
    
    @nodeFileName = 'coords_'+ @nodecount +'.xml'
    @areaPrefix = 'areas_'
    @referenceStrategy = 'sendAll'
    
    @maxPrefix = maxPrefix
    
  end

  def getMaxPrefix() 
    return @maxPrefix;
  end

  def getReferenceCoordsFilePath()
    return @gvbDataBaseDirectory + @referenceStrategy + '/' + @nodeFileName
  end
  
  def getReferenceAreaFilePath()
    return @gvbDataBaseDirectory + @referenceStrategy + '/' + @areaPrefix + @nodeFileName
  end

  def getTestCoordsFilePath()
    return @gvbDataBaseDirectory + @strategyName + '/' + @parameter * '/' + '/' + @nodeFileName
  end

  def getTestAreaFilePath()
    return @gvbDataBaseDirectory + @strategyName + '/' + @parameter * '/' + '/' + @areaPrefix  + @nodeFileName
  end
  
  def getNodeCount()
    return @nodecount;
  end
  
  def getStrategyParameter()
    return @parameter
  end
  
  def getStrategy()
    return @strategyName
  end
    
end
  


###
# Class Node
# Container for single Node
###
class Node
  
  def initialize()
    @coords
  end
  
  def setCoords(coords)
    @coords = coords
  end
  
  def getCoords()
    return @coords
  end
  
end

###
# Class Area
# Container for a single ara
###
class Area
  
  def initialize()
    @prefix = ''
    @dimensions = Hash.new
  end
  
  def setPrefix(prefix)
    @prefix = prefix
  end
  
  def setMaxBoundary(dim, max)
    @dimensions[dim]['max'] = max
  end
  
  def setMinBoundary(dim, min)
    if(@dimensions.has_key?(dim) == false)
      @dimensions[dim] = Hash.new
    end 
 
    @dimensions[dim]['min'] = min
  end
  
  def contains(node)
    
    dimCount = 0
    node.getCoords().each do |coord|
      if coord < @dimensions[dimCount]['min'] || coord > @dimensions[dimCount]['max'] 
        return false
      end
      dimCount+=1
    end
    
    return true
  end
  
end

###
# Class AreaMap
# 
###
class AreaMap
  def initialize()
    @areas = Hash.new()
  end
 
  def setFromXMLDoc(xmlDoc)
     
    prefix = 0
    
    xmlDoc.find("//arealist/area").each do |area|
        
    area.find('prefix').each do |prefixStr|
      prefix = prefixStr.content.to_s
    end
      
      
    if(@areas.has_key?(prefix) == false)   
        @areas[prefix] = Area.new
        @areas[prefix].setPrefix(prefix)
    end
    
    area.find('min').each do |minBoundary|
      @areas[prefix].setMinBoundary(minBoundary['dimension'].to_i, minBoundary.content.to_f)
    end
    
    area.find('max').each do |maxBoundary|
      @areas[prefix].setMaxBoundary(maxBoundary['dimension'].to_i, maxBoundary.content.to_f)
    end 
   end 
  end
  
  def getAreaCodeForNode(node)
    @areas.each do |areaCode, area|
      if area.contains(node)
        return areaCode
      end
    end
    
    puts "No Area found for node " + node.getCoords.to_s
    return '-1'
  end
  
end


###
# Class AreaEvaluate
# 
###
class AreaEvaluate
  

  def initialize(configObject)
   
    
    @configObject = configObject
    
    #
    # Key: length of prefix that NOT match: 0-maxprefix
    # Value: weighting 1-n
    # 
    #
    @accuracyRating = Hash.new(0) 
    @accuracyRating[0] = 11
    @accuracyRating[1] = 10
    @accuracyRating[2] = 9
    @accuracyRating[3] = 8
    @accuracyRating[4] = 7
    @accuracyRating[5] = 6
    @accuracyRating[6] = 5
    @accuracyRating[7] = 4  
    @accuracyRating[8] = 3
    @accuracyRating[9] = 2
    @accuracyRating[10] = 1
    @accuracyRating[11] = 0
    
    @maxBitError = 0
    @accuracyIndex = Hash.new
    @accuracySummary = {'index' => 0, 'percentage' => 0.0, 'bitErrorCount' => 0, 'averageBitError' => 0}
    
    @referenceNodes = Array.new
    
    loadXMLData(configObject.getReferenceCoordsFilePath).find("//nodelist/node").each do |nodeXML|
      
      coords = []
      nodeXML.find('coord').each do |coordXML|
          coords << coordXML.content.to_f
      end
      
      node = Node.new
      node.setCoords(coords)
      @referenceNodes << node
      
    end
    
    

    @referenceAreaMap = AreaMap.new
    @referenceAreaMap.setFromXMLDoc(loadXMLData(configObject.getReferenceAreaFilePath))
    
    @testAreaMap = AreaMap.new
    @testAreaMap.setFromXMLDoc(loadXMLData(configObject.getTestAreaFilePath))
    
  end
 
 
  
  def loadXMLData(fileName)
    xmlfile = File.read(fileName)
    parser = XML::Parser.string(xmlfile)
    nodeDoc = parser.parse
    return nodeDoc
  end
  
    
  
  def evaluate()
    
    index = getMaxPosibleIndex
    
    @referenceNodes.each do |referenceNode|
      
      refAreaCode = @referenceAreaMap.getAreaCodeForNode(referenceNode)
      testAreaCode = @testAreaMap.getAreaCodeForNode(referenceNode)
      
      notMatching = getNotMatchingPostfix(refAreaCode, testAreaCode)
      
      if notMatching > @maxBitError
        @maxBitError = notMatching
      end   
      
      if @accuracyIndex.has_key?(notMatching) == false
        @accuracyIndex[notMatching] = {'count' => 0, 'percentage' => 0.0}
      end
      
      @accuracyIndex[notMatching]['count']+=1
      
    end
    
    @accuracyIndex.each do |notMatch, hash|
      @accuracyIndex[notMatch]['percentage'] = (@accuracyIndex[notMatch]['count'].to_f / getNodeCount().to_f) * 100
      @accuracySummary['index'] += @accuracyIndex[notMatch]['count'] * @accuracyRating[notMatch]
      @accuracySummary['bitErrorCount'] += @accuracyIndex[notMatch]['count'] * notMatch;
    end
    
    @accuracySummary['averageBitError'] =  @accuracySummary['bitErrorCount'].to_f  / getNodeCount().to_f
    @accuracySummary['percentage'] = (@accuracySummary['index'].to_f / getMaxPosibleIndex.to_f) * 100
    
  end
  
  
  def printStats()
    
    puts '###### STATS for ' + @configObject.getNodeCount() + ' ######'
    
    i=0
    while i <= @maxBitError
       
      if @accuracyIndex.has_key?(i)
        count = @accuracyIndex[i]['count']
        percentage = @accuracyIndex[i]['percentage']
      else
        count = 0
        percentage = 0
      end
 
       puts i.to_s + ':' +   count.to_s + "(" + percentage.to_s + "%)"
       i += 1
    end
    
    #@accuracyIndex.each do |notMatch, hash|
    #  puts notMatch.to_s + ':' +   hash['count'].to_s + "(" + hash['percentage'].to_s + "%)"
    #end
    
    puts '------------------'
    puts @accuracySummary['index'].to_s + ' of ' + getMaxPosibleIndex().to_s + ' -> ' +  @accuracySummary['percentage'].to_s + '%'
    puts 'AverageBitError: ' + @accuracySummary['averageBitError'].to_s;
    puts '##################'    
  end
  
  
  
  def saveStats(outfile)
  
  outArray = Array.new()
  
  outArray << @configObject.getStrategy()
  outArray << @configObject.getNodeCount()
  
  # write parameters
  i=0
  parameter = @configObject.getStrategyParameter()
  while i < 3
    if(parameter.size > i)
      outArray << parameter[i]
    else
      outArray << ''
    end
    i += 1
  end
  
  outArray << # write average bit error
  @accuracySummary['averageBitError']
  
  # write values
  j=0
  while j <= @maxBitError
     
    if @accuracyIndex.has_key?(j)
      count = @accuracyIndex[j]['count']
      else
        count = 0
      end
      
      outArray << count
      j = j+1
  end  
    
    
  FasterCSV.open(outfile, "a") do |csv|
    csv << outArray
  end

    
  end
  
  
  
  
  def getNodeCount()
    return @referenceNodes.length
  end
  
  
  
  def getMaxPosibleIndex()
    return @accuracyRating[0] * getNodeCount()
  end
  
  
  
  protected
  
    def getNotMatchingPostfix(code1, code2)
    
      i = 0
      length = [code1.length, code2.length].min
      while i < length 
        matching = length - i
        if code1[0, matching] == code2[0, matching]
          return i
        end
        i+=1 
      end
      return i;
  end
  
  
end



######################## MAIN #################################

configObjectCollection = EvaluationConfigCollection.new

configObjectCollection.createCollection(gvbDataBaseDirectory, strategyName, nodecount, parameter, maxprefix).each do |configObject|

  puts "Testing " + configObject.getReferenceCoordsFilePath + " against " + configObject.getTestCoordsFilePath
  
  puts "Building Area Maps... "
  c2aParams = '-p ' + maxprefix.to_s
  system("ruby c2a.rb " + c2aParams + " " + configObject.getReferenceCoordsFilePath)
  system("ruby c2a.rb " + c2aParams + " " + configObject.getTestCoordsFilePath)
  
  # calculate accuracy index
  puts "Calculating accuracy index..."
  
  areaEvaluator = AreaEvaluate.new(configObject);
  puts "...done!"
  
  index = areaEvaluator.evaluate
  areaEvaluator.printStats
  areaEvaluator.saveStats("./stats_"+strategyName+"_"+nodecount.to_s+"_"+maxprefix.to_s+".csv")

end






