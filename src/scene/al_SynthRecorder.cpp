
#include "al/scene/al_SynthRecorder.hpp"

using namespace al;

void SynthRecorder::startRecord(std::string name, bool overwrite,
                                std::string tempo, bool quantize, std::string note,
                                bool startOnEvent) {
  mOverwrite = overwrite;
  mStartOnEvent = startOnEvent;
  mRecording = true;
  mSequenceStart = std::chrono::high_resolution_clock::now();
  mSequenceName = name;

  mQuantize = quantize;
  mNote = std::stoi(note);
  mTempo = std::stoi(tempo);
}

// quantizing to quarter note = tempo
double SynthRecorder::quantize(double time){
    if(!mQuantize){
        return time;
    }

    std::cout << "---" << time << "---" << std::endl;
    double barLength = 60./mTempo * 4;

    // times of eight eighth notes
    std::vector<double> noteVec;
    for(int i = 0; i < mNote; i++){
        noteVec.push_back(i * barLength/mNote/1.);
    }
    
    for(int i = 0; i < noteVec.size(); i++){
        std::cout << noteVec[i] << std::endl;
    }

    double time1 = time;
    int bars = 0;
    while(time1 > barLength){
        time1 -= barLength;
        bars++;
    }
    std::cout << "time in first bar: " << time1 << std::endl;

    // get difference in time to each of the eighth notes
    std::vector<double> diffVec;
    // double m_1, m_2, m_3, m_4, m_5, m_6, m_7, m_8;
    for(int i = 0; i < noteVec.size(); i++){
        // std::cout << "eighth: " << noteVec[i] << ", diff: " << std::abs(noteVec[i] - time1) << std::endl;
        diffVec.push_back(std::abs(noteVec[i] - time1));
    }

    // get minimum difference, set quantized time
    auto minDiff = std::min_element(diffVec.begin(), diffVec.end());
    double closestEighth = noteVec[minDiff - diffVec.begin()];
    std::cout << "closest eighth: " << closestEighth << std::endl;
    std::cout << bars * barLength + closestEighth << std::endl;
    return std::abs(bars * barLength + closestEighth - time) < std::abs(bars * barLength - time) ?
        std::abs(bars * barLength + closestEighth - time) < std::abs((bars + 1) * barLength - time) ? 
        bars * barLength + closestEighth : (bars + 1) * barLength : 
        std::abs(bars * barLength - time) < std::abs((bars + 1) * barLength - time) ? 
        bars * barLength : (bars + 1) * barLength;
}

void SynthRecorder::stopRecord() {
  mRecording = false;
  std::string path = File::conformDirectory(mDirectory);
  std::string fileName = path + mSequenceName + ".synthSequence";

  std::string newSequenceName = mSequenceName;
  if (!mOverwrite) {
    std::string newFileName = fileName;
    int counter = 0;
    while (File::exists(newFileName)) {
      newSequenceName = mSequenceName + "_" + std::to_string(counter++);
      newFileName = path + newSequenceName + ".synthSequence";
    }
    fileName = newFileName;
  }
  std::vector<std::string> usedInstruments;
  std::ofstream f(fileName);
  if (!f.is_open()) {
    std::cout << "Error while opening sequence file: " << fileName << std::endl;
    return;
  }
  if (mFormat == CPP_FORMAT) {
    for (SynthEvent &event : mSequence) {
      f << "s.add<" << event.synthName << ">(" << event.time << ").set(";
      for (unsigned int i = 0; i < event.pFields.size(); i++) {
        if (event.pFields[i].type() == ParameterField::STRING) {
          f << "\"" << event.pFields[i].get<std::string>() << "\"";
        } else {
          f << event.pFields[i].get<float>();
        }
        if (i < event.pFields.size() - 1) {
          f << ", ";
        }
      }
      f << ");" << std::endl;
      if (std::find(usedInstruments.begin(), usedInstruments.end(),
                    event.synthName) == usedInstruments.end()) {
        usedInstruments.push_back(event.synthName);
      }
    }
  } else if (mFormat == SEQUENCER_EVENT) {
    std::map<int, SynthEvent *> eventStack;
    for (SynthEvent &event : mSequence) {
      if (event.type == SynthEventType::TRIGGER_ON) {
        eventStack[event.id] = &event;
      } else if (event.type == SynthEventType::TRIGGER_OFF) {
        auto idMatch = eventStack.find(event.id);
        if (idMatch != eventStack.end()) {
          double duration = event.time - idMatch->second->time;
          f << "@ " << quantize(idMatch->second->time) << " " << duration << " "
            << idMatch->second->synthName << " ";
          for (auto &field : idMatch->second->pFields) {
            if (field.type() == ParameterField::STRING) {
              f << "\"" << field.get<std::string>() << "\" ";
            } else {
              f << field.get<float>() << " ";
            }
          }
          f << std::endl;
        }
      }
      if (std::find(usedInstruments.begin(), usedInstruments.end(),
                    event.synthName) == usedInstruments.end()) {
        usedInstruments.push_back(event.synthName);
      }
    }
    if (eventStack.size() > 0) {
      std::cout << "WARNING: event stack not empty (trigger on doesn't have a "
                   "trigger off match)"
                << std::endl;
    }

  } else if (mFormat == SEQUENCER_TRIGGERS) {
    for (SynthEvent &event : mSequence) {
      if (event.type == SynthEventType::TRIGGER_ON) {
        f << "+ " << event.time << " " << event.id << " " << event.synthName
          << " ";
        for (auto field : event.pFields) {
          if (field.type() == ParameterField::STRING) {
            f << "\"" << field.get<std::string>() << "\"";
          } else {
            f << field.get<float>() << " ";
          }
        }
        f << std::endl;
      } else if (event.type == SynthEventType::TRIGGER_OFF) {
        f << "- " << event.time << " " << event.id << " ";
        for (unsigned int i = 0; i < event.pFields.size(); i++) {
          if (event.pFields[i].type() == ParameterField::STRING) {
            f << "\"" << event.pFields[i].get<std::string>() << "\" ";
          } else {
            f << event.pFields[i].get<float>() << " ";
          }
        }
        f << std::endl;
      }
      if (std::find(usedInstruments.begin(), usedInstruments.end(),
                    event.synthName) == usedInstruments.end()) {
        usedInstruments.push_back(event.synthName);
      }
    }
  }

  if (f.bad()) {
    std::cout << "Error while writing sequence file: " << fileName << std::endl;
  }
  mSequence.clear();
  for (auto &instr : usedInstruments) {
    f << "# " << instr << " ";
    // Hack to get the parameter names. Get a voice from the polysynth and then
    // check the parameters. Should there be a better way?
    auto *voice = mPolySynth->getVoice(instr);
    for (auto p : voice->triggerParameters()) {
      f << p->getName() << " ";
    }
    f << std::endl;
    mPolySynth->insertFreeVoice(voice);
  }
  f.close();

  std::cout << "Recorded: " << fileName << std::endl;
  //        recorder->mLastSequenceName = newSequenceName;
  //        recorder->mLastSequenceSubDir =
  //        recorder->mPresetHandler->getSubDirectory();
}

void SynthRecorder::registerPolySynth(PolySynth &polySynth) {
  polySynth.registerTriggerOnCallback(SynthRecorder::onTriggerOn, this);
  polySynth.registerTriggerOffCallback(SynthRecorder::onTriggerOff, this);
  mPolySynth = &polySynth;
}
